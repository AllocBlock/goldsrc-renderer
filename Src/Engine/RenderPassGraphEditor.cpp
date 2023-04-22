#include "RenderPassGraphEditor.h"

void IRenderPassGraphEditCommand::execute(ptr<SRenderPassGraph> vGraph)
{
    if (m_Executed) throw std::runtime_error("Already executed");
    m_pGraph = vGraph;
    _executeV(m_pGraph.lock());
    m_Executed = true;
}

void IRenderPassGraphEditCommand::withdraw()
{
    if (!m_Executed) throw std::runtime_error("Not executed yet");
    _withdrawV(m_pGraph.lock());
    m_Executed = false;
    m_pGraph.reset();
}

CCommandAddLink::CCommandAddLink(size_t vLinkId, const SRenderPassGraphLink& vLink)
{
    m_LinkId = vLinkId;
    m_Link = vLink;
}

void CCommandAddLink::_executeV(ptr<SRenderPassGraph> vGraph)
{
    _ASSERTE(vGraph->hasNode(m_Link.Source.NodeId));
    _ASSERTE(vGraph->hasNode(m_Link.Destination.NodeId));
    _ASSERTE(!vGraph->hasLink(m_LinkId));
    // Remove conflict link
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
    vGraph->LinkMap[m_LinkId] = m_Link;
}

void CCommandAddLink::_withdrawV(ptr<SRenderPassGraph> vGraph)
{
    _ASSERTE(vGraph->hasLink(m_LinkId));
    vGraph->LinkMap.erase(m_LinkId);
    if (m_ReplacedLink.has_value())
    {
        vGraph->LinkMap[m_ReplacedLink->first] = m_ReplacedLink->second;
        m_ReplacedLink.reset();
    }
}

CCommandRemoveLink::CCommandRemoveLink(size_t vLinkId)
{
    m_LinkId = vLinkId;
}

void CCommandRemoveLink::_executeV(ptr<SRenderPassGraph> vGraph)
{
    _ASSERTE(vGraph->hasLink(m_LinkId));

    m_Link = std::move(vGraph->LinkMap.at(m_LinkId));
    vGraph->LinkMap.erase(m_LinkId);
}

void CCommandRemoveLink::_withdrawV(ptr<SRenderPassGraph> vGraph)
{
    _ASSERTE(!vGraph->hasLink(m_LinkId));
    vGraph->LinkMap[m_LinkId] = m_Link;
}

CCommandRemoveNode::CCommandRemoveNode(size_t vNodeId)
{
    m_NodeId = vNodeId;
    m_Node = SRenderPassGraphNode();
}

void CCommandRemoveNode::_executeV(ptr<SRenderPassGraph> vGraph)
{
    _ASSERTE(vGraph->hasNode(m_NodeId));
    m_Node = std::move(vGraph->NodeMap.at(m_NodeId));

    // remove node
    vGraph->NodeMap.erase(m_NodeId);

    // remove related links
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

    // remove entry if needed
    if (vGraph->EntryPortOpt.has_value() && vGraph->EntryPortOpt->NodeId == m_NodeId)
    {
        m_Entry = vGraph->EntryPortOpt;
        vGraph->EntryPortOpt.reset();
    }
}

void CCommandRemoveNode::_withdrawV(ptr<SRenderPassGraph> vGraph)
{
    _ASSERTE(!vGraph->hasNode(m_NodeId));
    vGraph->NodeMap[m_NodeId] = m_Node;
    vGraph->LinkMap.insert(m_LinkMap.begin(), m_LinkMap.end());
    if (m_Entry.has_value())
    {
        vGraph->EntryPortOpt = m_Entry;
        m_Entry.reset();
    }
}

CCommandSetEntry::CCommandSetEntry(size_t vNodeId, const std::string& vPortName)
{
    m_NodeId = vNodeId;
    m_PortName = vPortName;
}

void CCommandSetEntry::_executeV(ptr<SRenderPassGraph> vGraph)
{
    m_OldEntry = vGraph->EntryPortOpt;
    vGraph->EntryPortOpt = { m_NodeId, m_PortName };
}

void CCommandSetEntry::_withdrawV(ptr<SRenderPassGraph> vGraph)
{
    vGraph->EntryPortOpt = m_OldEntry;
}

void CRenderPassGraphEditor::setGraph(ptr<SRenderPassGraph> vGraph)
{
    m_pGraph = vGraph;
    m_CurNodeId = m_CurLinkId = 0;
    for (const auto& Pair : m_pGraph->NodeMap)
        m_CurNodeId = std::max(m_CurNodeId, Pair.first + 1);
    for (const auto& Pair : m_pGraph->LinkMap)
        m_CurLinkId = std::max(m_CurLinkId, Pair.first + 1);
    clearHistory();
}

void CRenderPassGraphEditor::execCommand(IRenderPassGraphEditCommand::Ptr vCommand, bool vEnableUndo)
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

bool CRenderPassGraphEditor::canUndo()
{
    return m_pCurCommand != m_CommandLinkList.begin();
}

void CRenderPassGraphEditor::undo()
{
    if (m_pCurCommand == m_CommandLinkList.begin()) throw std::runtime_error("No command for undo");
    m_pCurCommand = std::prev(m_pCurCommand);
    (*m_pCurCommand)->withdraw();
}

bool CRenderPassGraphEditor::canRedo()
{
    return m_pCurCommand != m_CommandLinkList.end();
}

void CRenderPassGraphEditor::redo()
{
    if (m_pCurCommand == m_CommandLinkList.end()) throw std::runtime_error("No command for redo");
    (*m_pCurCommand)->execute(m_pGraph);
    m_pCurCommand = std::next(m_pCurCommand);
}

void CRenderPassGraphEditor::clearHistory()
{
    m_CommandLinkList.clear();
    m_pCurCommand = m_CommandLinkList.end();
}

void CRenderPassGraphEditor::addLink(const SRenderPassGraphLink& vLink)
{
    execCommand(make<CCommandAddLink>(m_CurLinkId++, vLink));
}

void CRenderPassGraphEditor::addLink(const SRenderPassGraphPortInfo& vSourcePort,
    const SRenderPassGraphPortInfo& vDestPort)
{
    SRenderPassGraphLink Link = { vSourcePort, vDestPort };
    addLink(Link);
}

void CRenderPassGraphEditor::addLink(size_t vStartNodeId, const std::string& vStartPortName, size_t vEndNodeId,
    const std::string& vEndPortName)
{
    addLink({ vStartNodeId, vStartPortName }, { vEndNodeId, vEndPortName });
}

void CRenderPassGraphEditor::removeNode(size_t vNodeId)
{ execCommand(make<CCommandRemoveNode>(vNodeId)); }

void CRenderPassGraphEditor::removeLink(size_t vLinkId)
{ execCommand(make<CCommandRemoveLink>(vLinkId)); }

void CRenderPassGraphEditor::setEntry(size_t vNodeId, const std::string & vPortName)
{ execCommand(make<CCommandSetEntry>(vNodeId, vPortName)); }

bool CRenderPassGraphEditor::__hasPass(size_t vNodeId)
{
    return m_pGraph->NodeMap.find(vNodeId) != m_pGraph->NodeMap.end();
}

void CRenderPassGraphEditor::__clear()
{
    m_pGraph->NodeMap.clear();
    m_pGraph->LinkMap.clear();
    m_pGraph->EntryPortOpt = std::nullopt;
}
