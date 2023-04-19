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

    void execute(ptr<SRenderPassGraph> vGraph);
    void withdraw();

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
    CCommandAddLink(size_t vLinkId, const SRenderPassGraphLink& vLink);

protected:
    virtual void _executeV(ptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override;

private:
    size_t m_LinkId;
    SRenderPassGraphLink m_Link;
    std::optional<std::pair<size_t, SRenderPassGraphLink>> m_ReplacedLink; // one input allow only one source, old source will be replaced
};

class CCommandRemoveLink : public IRenderPassGraphEditCommand
{
public:
    CCommandRemoveLink(size_t vLinkId);

protected:
    virtual void _executeV(ptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override;

private:
    size_t m_LinkId;
    SRenderPassGraphLink m_Link;
};

class CCommandRemoveNode : public IRenderPassGraphEditCommand
{
public:
    CCommandRemoveNode(size_t vNodeId);

protected:
    virtual void _executeV(ptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(ptr<SRenderPassGraph> vGraph) override;

private:
    size_t m_NodeId;
    SRenderPassGraphNode m_Node;
    std::map<size_t, SRenderPassGraphLink> m_LinkMap;
};

class CRenderPassGraphEditor
{
public:
    void setGraph(ptr<SRenderPassGraph> vGraph);
    void execCommand(IRenderPassGraphEditCommand::Ptr vCommand, bool vEnableUndo = true);
    bool canUndo();
    void undo();
    bool canRedo();
    void redo();
    void clearHistory();

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

    void addLink(const SRenderPassGraphLink& vLink);
    void addLink(const SRenderPassGraphPortInfo& vSourcePort, const SRenderPassGraphPortInfo& vDestPort);
    void addLink(size_t vStartNodeId, const std::string& vStartPortName, size_t vEndNodeId,
                 const std::string& vEndPortName);
    void removeNode(size_t vNodeId);
    void removeLink(size_t vLinkId);

    Math::SAABB2D getAABB() const;

private:
    bool __hasPass(size_t vNodeId);
    void __clear();

    ptr<SRenderPassGraph> m_pGraph = nullptr;
    
    size_t m_CurNodeId = 0;
    size_t m_CurLinkId = 0;

    size_t m_MaxUndoCount = 50;
    std::list<IRenderPassGraphEditCommand::Ptr> m_CommandLinkList;
    std::list<IRenderPassGraphEditCommand::Ptr>::iterator m_pCurCommand;
};