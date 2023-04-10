#pragma once
#include "Pointer.h"
#include "RenderPassGraph.h"

#include <list>
#include <stdexcept>

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

// TODO: handle if node has entry port
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
        m_Node = std::move(vGraph->NodeMap.at(m_NodeId));
        vGraph->NodeMap.erase(m_NodeId);

        auto& GraphLinkSet = vGraph->LinkSet;
        for (size_t i = 0; i < GraphLinkSet.size(); ++i)
        {
            if (GraphLinkSet[i].Source.NodeId == m_NodeId || GraphLinkSet[i].Destination.NodeId == m_NodeId)
            {
                m_LinkSet.emplace_back(GraphLinkSet[i]);
                GraphLinkSet.erase(GraphLinkSet.begin() + i);
                --i;
            }
        }
    }

    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override
    {
        vGraph->NodeMap[m_NodeId] = m_Node;
        vGraph->LinkSet.insert(vGraph->LinkSet.end(), m_LinkSet.begin(), m_LinkSet.end());
    }

private:
    size_t m_NodeId;
    SRenderPassGraphNode m_Node;
    std::vector<SRenderPassGraphLink> m_LinkSet;
};

class CRenderPassGraphEditor
{
public:
    void setGraph(ptr<SRenderPassGraph> vGraph)
    {
        m_pGraph = vGraph;
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

    void deleteNode(size_t vNodeId)
    {
        execCommand(make<CCommandRemoveNode>(vNodeId));
    }

    void clearHistory()
    {
        m_CommandLinkList.clear();
        m_pCurCommand = m_CommandLinkList.end();
    }
    
private:
    ptr<SRenderPassGraph> m_pGraph = nullptr;
    size_t m_MaxUndoCount = 50;
    std::list<IRenderPassGraphEditCommand::Ptr> m_CommandLinkList;
    std::list<IRenderPassGraphEditCommand::Ptr>::iterator m_pCurCommand;
};