#pragma once
#include "Pointer.h"
#include "RenderPassGraph.h"

#include <list>
#include <stdexcept>

// TODO: check loop when add link
class IRenderPassGraphEditCommand
{
public:
    
    virtual ~IRenderPassGraphEditCommand() = default;

    void execute(sptr<SRenderPassGraph> vGraph);
    void withdraw();

protected:
    virtual void _executeV(sptr<SRenderPassGraph> vGraph) = 0;
    virtual void _withdrawV(sptr<SRenderPassGraph> vGraph) = 0;

    bool m_Executed = false;

private:
    wptr<SRenderPassGraph> m_pGraph;
};

class CCommandAddNode : public IRenderPassGraphEditCommand
{
public:
    CCommandAddNode(size_t vNodeId, const SRenderPassGraphNode& vNode);

protected:
    virtual void _executeV(sptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(sptr<SRenderPassGraph> vGraph) override;

private:
    size_t m_NodeId;
    SRenderPassGraphNode m_Node;
};

class CCommandAddLink : public IRenderPassGraphEditCommand
{
public:
    CCommandAddLink(size_t vLinkId, const SRenderPassGraphLink& vLink);

protected:
    virtual void _executeV(sptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(sptr<SRenderPassGraph> vGraph) override;

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
    virtual void _executeV(sptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(sptr<SRenderPassGraph> vGraph) override;

private:
    size_t m_LinkId;
    SRenderPassGraphLink m_Link;
};

class CCommandRemoveNode : public IRenderPassGraphEditCommand
{
public:
    CCommandRemoveNode(size_t vNodeId);

protected:
    virtual void _executeV(sptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(sptr<SRenderPassGraph> vGraph) override;

private:
    size_t m_NodeId;
    SRenderPassGraphNode m_Node;
    std::map<size_t, SRenderPassGraphLink> m_LinkMap;
    std::optional<SRenderPassGraphPortInfo> m_Output;
};

class CCommandSetEntry : public IRenderPassGraphEditCommand
{
public:
    CCommandSetEntry(size_t vNodeId, const std::string& vPortName);

protected:
    virtual void _executeV(sptr<SRenderPassGraph> vGraph) override;
    virtual void _withdrawV(sptr<SRenderPassGraph> vGraph) override;

private:
    size_t m_NodeId;
    std::string m_PortName;
    std::optional<SRenderPassGraphPortInfo> m_OldOutput;
};

class CRenderPassGraphEditor
{
public:
    void setGraph(sptr<SRenderPassGraph> vGraph);
    void execCommand(sptr<IRenderPassGraphEditCommand> vCommand, bool vEnableUndo = true);
    bool canUndo();
    void undo();
    bool canRedo();
    void redo();
    void clearHistory();

    void addNode(const std::string& vName, glm::vec2 vPos);
    void addLink(const SRenderPassGraphLink& vLink);
    void addLink(const SRenderPassGraphPortInfo& vSourcePort, const SRenderPassGraphPortInfo& vDestPort);
    void addLink(size_t vStartNodeId, const std::string& vStartPortName, size_t vEndNodeId,
                 const std::string& vEndPortName);
    void removeNode(size_t vNodeId);
    void removeLink(size_t vLinkId);
    void setEntry(size_t vNodeId, const std::string& vPortName);

private:
    bool __hasPass(size_t vNodeId);
    void __clear();

    sptr<SRenderPassGraph> m_pGraph = nullptr;
    
    size_t m_CurNodeId = 0;
    size_t m_CurLinkId = 0;

    size_t m_MaxUndoCount = 50;
    std::list<sptr<IRenderPassGraphEditCommand>> m_CommandLinkList;
    std::list<sptr<IRenderPassGraphEditCommand>>::iterator m_pCurCommand;
};