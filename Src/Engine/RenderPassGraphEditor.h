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
    }

    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override
    {
        _ASSERTE(vGraph->LinkMap.find(m_LinkId) != vGraph->LinkMap.end());
        vGraph->LinkMap.erase(m_LinkId);
    }

private:
    size_t m_LinkId;
    SRenderPassGraphLink m_Link;
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

    void addLink(const SRenderPassGraphPortInfo& vSourcePort, const SRenderPassGraphPortInfo& vDestPort)
    {
        SRenderPassGraphLink Link = { vSourcePort, vDestPort };
        execCommand(make<CCommandAddLink>(m_CurLinkId++, Link));
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

    // temp, remove this later
    static ptr<SRenderPassGraph> createFromRenderPassGraph(std::vector<vk::IRenderPass::Ptr> vPassSet,
        std::vector<std::tuple<int, std::string, int, std::string>> vLinks, std::pair<int, std::string> vEntry)
    {
        auto pGraph = make<SRenderPassGraph>();
        size_t CurNodeId = 0;

        pGraph->NodeMap[CurNodeId++] = SRenderPassGraphNode{ "Swapchain", glm::vec2(0, 0),glm::vec2(20.0f), {}, {"Main"} };

        std::vector<size_t> PassIds(vPassSet.size()); // index to id
        for (size_t i = 0; i < vPassSet.size(); ++i)
        {
            std::string PassName = vPassSet[i]->getNameV();

            auto pPortSet = vPassSet[i]->getPortSet();
            std::vector<std::string> InputPortSet, OutputPortSet;
            for (size_t i = 0; i < pPortSet->getInputPortNum(); ++i)
                InputPortSet.emplace_back(pPortSet->getInputPort(i)->getName());
            for (size_t i = 0; i < pPortSet->getOutputPortNum(); ++i)
                OutputPortSet.emplace_back(pPortSet->getOutputPort(i)->getName());

            size_t Id = CurNodeId;
            PassIds[i] = Id;

            pGraph->NodeMap[Id] = SRenderPassGraphNode{ PassName, glm::vec2(0, 0), glm::vec2(20.0f), InputPortSet, OutputPortSet };
            CurNodeId++;
        }

        // links
        size_t CurLinkId = 0;
        pGraph->EntryPortOpt = SRenderPassGraphPortInfo{ PassIds[vEntry.first], vEntry.second };
        pGraph->LinkMap[CurLinkId++] = SRenderPassGraphLink{ {0, "Main"}, {pGraph->EntryPortOpt->NodeId, pGraph->EntryPortOpt->Name} };

        for (const auto& Link : vLinks)
        {
            int StartPassIndex = std::get<0>(Link);
            std::string StartPortName = std::get<1>(Link);
            int EndPassIndex = std::get<2>(Link);
            std::string EndPortName = std::get<3>(Link);

            size_t StartNodeId = PassIds[StartPassIndex];
            size_t EndNodeId = PassIds[EndPassIndex];
            pGraph->LinkMap[CurLinkId++] = SRenderPassGraphLink{ {StartNodeId, StartPortName}, {EndNodeId, EndPortName} };
        }
        return pGraph;
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
    
    size_t m_CurNodeId = 0;
    size_t m_CurLinkId = 0;

    size_t m_MaxUndoCount = 50;
    std::list<IRenderPassGraphEditCommand::Ptr> m_CommandLinkList;
    std::list<IRenderPassGraphEditCommand::Ptr>::iterator m_pCurCommand;
};