#pragma once
#include "Pointer.h"
#include "RenderPassGraph.h"

#include <list>
#include <stdexcept>

// TODO: check loop when add link
class IRenderPassGraphEditCommand
{
public:
    _DEFINE_PTR(IRenderPassGraphEditCommand);

    virtual ~IRenderPassGraphEditCommand() = default;

    void execute(ptr<SRenderPassGraph> vGraph)
    {
        if (m_Executed) throw std::runtime_error("Already executed");
        m_pGraph = vGraph;
        _executeV(m_pGraph.lock());
        m_Executed = true;
    }
    void withdraw()
    {
        if (!m_Executed) throw std::runtime_error("Not executed yet");
        _withdrawV(m_pGraph.lock());
        m_Executed = false;
        m_pGraph.reset();
    }
protected:
    virtual void _executeV(ptr<SRenderPassGraph> vGraph) = 0;
    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) = 0;

    bool m_Executed = false;

private:
    wptr<SRenderPassGraph> m_pGraph;
};


class CCommandAddLink : public IRenderPassGraphEditCommand
{
public:
    CCommandAddLink(size_t vLinkId, const SRenderPassGraphLink& vLink)
    {
        m_LinkId = vLinkId;
        m_Link = vLink;
    }

protected:
    virtual void _executeV(ptr<SRenderPassGraph> vGraph) override
    {
        _ASSERTE(vGraph->NodeMap.find(m_Link.Source.NodeId) != vGraph->NodeMap.end());
        _ASSERTE(vGraph->NodeMap.find(m_Link.Destination.NodeId) != vGraph->NodeMap.end());
        _ASSERTE(vGraph->NodeMap.at(m_Link.Source.NodeId).hasOutput(m_Link.Source.Name));
        _ASSERTE(vGraph->NodeMap.at(m_Link.Destination.NodeId).hasInput(m_Link.Destination.Name));
        _ASSERTE(vGraph->LinkMap.find(m_LinkId) == vGraph->LinkMap.end());
        vGraph->LinkMap[m_LinkId] = m_Link;
        // FIXME: here assume AT MOST ONE LINK is on the source
        for (const auto& Pair : vGraph->LinkMap)
        {
            if (Pair.second.Destination == m_Link.Destination)
            {
                m_ReplacedLink = Pair;
                vGraph->LinkMap.erase(Pair.first);
                break;
            }
        }
    }

    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override
    {
        _ASSERTE(vGraph->LinkMap.find(m_LinkId) != vGraph->LinkMap.end());
        vGraph->LinkMap.erase(m_LinkId);
        if (m_ReplacedLink.has_value())
        {
            vGraph->LinkMap[m_ReplacedLink->first] = m_ReplacedLink->second;
            m_ReplacedLink.reset();
        }
    }

private:
    size_t m_LinkId;
    SRenderPassGraphLink m_Link;
    std::optional<std::pair<size_t, SRenderPassGraphLink>> m_ReplacedLink; // one input allow only one source, old source will be replaced
};

class CCommandRemoveLink : public IRenderPassGraphEditCommand
{
public:
    CCommandRemoveLink(size_t vLinkId)
    {
        m_LinkId = vLinkId;
    }

protected:
    virtual void _executeV(ptr<SRenderPassGraph> vGraph) override
    {
        _ASSERTE(vGraph->LinkMap.find(m_LinkId) != vGraph->LinkMap.end());

        m_Link = std::move(vGraph->LinkMap.at(m_LinkId));
        vGraph->LinkMap.erase(m_LinkId);
    }

    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override
    {
        _ASSERTE(vGraph->LinkMap.find(m_LinkId) == vGraph->LinkMap.end());
        vGraph->LinkMap[m_LinkId] = m_Link;
    }

private:
    size_t m_LinkId;
    SRenderPassGraphLink m_Link;
};

class CCommandRemoveNode : public IRenderPassGraphEditCommand
{
public:
    CCommandRemoveNode(size_t vNodeId)
    {
        m_NodeId = vNodeId;
        m_Node = SRenderPassGraphNode();
    }

protected:
    virtual void _executeV(ptr<SRenderPassGraph> vGraph) override
    {
        _ASSERTE(vGraph->NodeMap.find(m_NodeId) != vGraph->NodeMap.end());
        m_Node = std::move(vGraph->NodeMap.at(m_NodeId));
        vGraph->NodeMap.erase(m_NodeId);
        
        for (auto pIter = vGraph->LinkMap.begin(); pIter != vGraph->LinkMap.end();)
        {
            size_t LinkId = pIter->first;
            const SRenderPassGraphLink& Link = pIter->second;

            if (Link.Source.NodeId == m_NodeId || Link.Destination.NodeId == m_NodeId)
            {
                m_LinkMap[LinkId] = Link;
                pIter = vGraph->LinkMap.erase(pIter);
            }
            else
                ++pIter;
        }
    }

    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override
    {
        _ASSERTE(vGraph->NodeMap.find(m_NodeId) == vGraph->NodeMap.end());
        vGraph->NodeMap[m_NodeId] = m_Node;
        vGraph->LinkMap.insert(m_LinkMap.begin(), m_LinkMap.end());
    }

private:
    size_t m_NodeId;
    SRenderPassGraphNode m_Node;
    std::map<size_t, SRenderPassGraphLink> m_LinkMap;
};

class CRenderPassGraphEditor
{
public:
    void setGraph(ptr<SRenderPassGraph> vGraph)
    {
        m_pGraph = vGraph;
        m_CurNodeId = m_CurLinkId = 0;
        for (const auto& Pair : m_pGraph->NodeMap)
            m_CurNodeId = std::max(m_CurNodeId, Pair.first + 1);
        for (const auto& Pair : m_pGraph->LinkMap)
            m_CurLinkId = std::max(m_CurLinkId, Pair.first + 1);
        clearHistory();
    }

    void execCommand(IRenderPassGraphEditCommand::Ptr vCommand, bool vEnableUndo = true)
    {
        vCommand->execute(m_pGraph);
        if(m_pCurCommand != m_CommandLinkList.end())
        {
            m_CommandLinkList.erase(m_pCurCommand, m_CommandLinkList.end()); // remove original redo commands
            m_pCurCommand = m_CommandLinkList.end();
        }

        if (vEnableUndo)
        {
            if (m_CommandLinkList.size() >= m_MaxUndoCount)
                m_CommandLinkList.pop_front();
            m_CommandLinkList.push_back(vCommand);
            m_pCurCommand = m_CommandLinkList.end();
        }
    }

    bool canUndo() {
        return m_pCurCommand != m_CommandLinkList.begin();
    }
    
    void undo()
    {
        if (m_pCurCommand == m_CommandLinkList.begin()) throw std::runtime_error("No command for undo");
        m_pCurCommand = std::prev(m_pCurCommand);
        (*m_pCurCommand)->withdraw();
    }

    bool canRedo() {
        return m_pCurCommand != m_CommandLinkList.end();
    }

    void redo()
    {
        if (m_pCurCommand == m_CommandLinkList.end()) throw std::runtime_error("No command for redo");
        (*m_pCurCommand)->execute(m_pGraph);
        m_pCurCommand = std::next(m_pCurCommand);
    }

    /*size_t addNode(const std::string& vName, const std::vector<std::string>& vInputSet,
        const std::vector<std::string>& vOutputSet)
    {
        const float Margin = 20.0f;

        SAABB2D AABB = getAABB();

        glm::vec2 Pos = glm::vec2(AABB.Max.x + Margin, (AABB.Min.y + AABB.Max.y) * 0.5f);

        SRenderPassGraphNode Node;
        Node.Name = vName;
        Node.Pos = Pos;
        Node.InputSet = vInputSet;
        Node.OutputSet = vOutputSet;

        if (!m_AABB.has_value())
            m_AABB = Node.getAABB();
        else
            m_AABB.value().applyUnion(Node.getAABB());

        m_pGraph->NodeMap[m_CurNodeIndex] = std::move(Node);
        return m_CurNodeIndex++;
    }*/

    void addLink(const SRenderPassGraphLink& vLink)
    {
        execCommand(make<CCommandAddLink>(m_CurLinkId++, vLink));
    }

    void addLink(const SRenderPassGraphPortInfo& vSourcePort, const SRenderPassGraphPortInfo& vDestPort)
    {
        SRenderPassGraphLink Link = { vSourcePort, vDestPort };
        addLink(Link);
    }

    void addLink(size_t vStartNodeId, const std::string& vStartPortName, size_t vEndNodeId,
        const std::string& vEndPortName)
    {
        addLink({ vStartNodeId, vStartPortName }, { vEndNodeId, vEndPortName });
    }

    void removeNode(size_t vNodeId)
    { execCommand(make<CCommandRemoveNode>(vNodeId)); }
    void removeLink(size_t vLinkId)
    { execCommand(make<CCommandRemoveLink>(vLinkId)); }

    void clearHistory()
    {
        m_CommandLinkList.clear();
        m_pCurCommand = m_CommandLinkList.end();
    }

    SAABB2D getAABB()const
    {
        if (m_pGraph && !m_pGraph->NodeMap.empty())
        {
            SAABB2D AABB = m_pGraph->NodeMap.begin()->second.getAABB();
            for (auto pIter = std::next(m_pGraph->NodeMap.begin()); pIter != m_pGraph->NodeMap.end(); ++pIter)
            {
                AABB.applyUnion(pIter->second.getAABB());
            }
            return AABB;
        }
        else
            return SAABB2D(glm::vec2(0, 0), glm::vec2(0, 0));
    }
private:
    bool hasPass(size_t vNodeId)
    {
        return m_pGraph->NodeMap.find(vNodeId) != m_pGraph->NodeMap.end();
    }

    void clear()
    {
        m_pGraph->NodeMap.clear();
        m_pGraph->LinkMap.clear();
        m_pGraph->EntryPortOpt = std::nullopt;
    }

    ptr<SRenderPassGraph> m_pGraph = nullptr;
    
    size_t m_CurNodeId = SRenderPassGraph::SwapchainNodeId;
    size_t m_CurLinkId = 0;

    size_t m_MaxUndoCount = 50;
    std::list<IRenderPassGraphEditCommand::Ptr> m_CommandLinkList;
    std::list<IRenderPassGraphEditCommand::Ptr>::iterator m_pCurCommand;
};