#include "RenderPassGraphAddLinkState.h"
#include "Maths.h"

#include <stdexcept>

void CRenderGraphAddLinkState::setGraph(ptr<SRenderPassGraph> vGraph)
{
    m_pGraph = vGraph;
    if (isStarted())
        end();
}

void CRenderGraphAddLinkState::start(const SRenderPassGraphPortInfo& vStartPort, bool vIsSource)
{
    if (!m_pGraph)
        throw std::runtime_error("Graph is not set");
    m_FixedPort = vStartPort;
    m_IsFixedPortSource = vIsSource;
    m_IsAdding = true;
}

void CRenderGraphAddLinkState::clearCandidates()
{
    __assertStarted();
    m_AttachedPort.reset();
    m_AttachedPortPriority = -INFINITY;
    m_IsAttachedPortSource = false;
}

bool CRenderGraphAddLinkState::addCandidate(const SRenderPassGraphPortInfo& vTargetPort, bool vIsSource,
    float vPriority)
{
    __assertStarted();

    // skip if it's the same node as fixed port
    if (vTargetPort.NodeId == m_FixedPort.NodeId) return false;

    // skip if already have this link
    if (m_IsFixedPortSource != m_IsAttachedPortSource)
    {
        SRenderPassGraphLink Link = __generateLink(vTargetPort);
        if (m_pGraph->hasLink(Link)) return false;
    }

    // keep most important port
    if (!m_AttachedPort.has_value() || vPriority > m_AttachedPortPriority)
    {
        m_AttachedPort = vTargetPort;
        m_AttachedPortPriority = vPriority;
        m_IsAttachedPortSource = vIsSource;
    }
    return true;
}

bool CRenderGraphAddLinkState::isStarted() const
{
    return m_IsAdding;
}

const SRenderPassGraphPortInfo& CRenderGraphAddLinkState::getFixedPort()
{ __assertStarted(); return m_FixedPort; }

bool CRenderGraphAddLinkState::isFixedPortSource()
{ __assertStarted(); return m_IsFixedPortSource; }

EAddLinkAttachState CRenderGraphAddLinkState::getLinkState(std::string& voReason) const
{
    if (!m_AttachedPort.has_value())
    {
        return EAddLinkAttachState::NOT_ATTACHED;
    }
    if (m_IsFixedPortSource == m_IsAttachedPortSource) // input-input or output-output
    {
        voReason = u8"不能连接同侧的Port";
        return EAddLinkAttachState::INVALID_ATTACH;
    }

    // if there is loop dependency
    Math::CDirectedGraph DirectedGraph;
    for (const auto& Pair : m_pGraph->NodeMap)
    {
        DirectedGraph.addNode(Pair.first);
    }
    size_t NewSourceNodeId = m_FixedPort.NodeId;
    size_t NewDestNodeId = m_AttachedPort->NodeId;
    if (!m_IsFixedPortSource)
        std::swap(NewSourceNodeId, NewDestNodeId);
    DirectedGraph.addEdge(NewDestNodeId, NewSourceNodeId);

    for (const auto& Pair : m_pGraph->LinkMap)
    {
        const SRenderPassGraphLink& Link = Pair.second;
        DirectedGraph.addEdge(Link.Destination.NodeId, Link.Source.NodeId);
    }
    if (DirectedGraph.hasLoop())
    {
        voReason = u8"禁止循环依赖";
        return EAddLinkAttachState::INVALID_ATTACH;
    }
    
    return EAddLinkAttachState::VALID_ATTACH;
}

const SRenderPassGraphPortInfo& CRenderGraphAddLinkState::getCurrentAttachedPort()
{
    if (!m_AttachedPort.has_value())
        throw std::runtime_error("No attached port for now, call getLinkState first to check if any port is attached");
    return m_AttachedPort.value();
}

SRenderPassGraphLink CRenderGraphAddLinkState::getCurrentValidLink()
{
    std::string Message;
    if (getLinkState(Message) != EAddLinkAttachState::VALID_ATTACH)
        throw std::runtime_error("No valid attached link for now, call getLinkState first to check");

    return __generateLink(m_AttachedPort.value());
}

void CRenderGraphAddLinkState::end()
{
    m_IsAdding = false;
}

void CRenderGraphAddLinkState::__assertStarted()
{
    if (!m_IsAdding)
        throw std::runtime_error("Not started");
}

SRenderPassGraphLink CRenderGraphAddLinkState::__generateLink(const SRenderPassGraphPortInfo& vAttachedPort)
{
    __assertStarted();
    if (m_IsFixedPortSource == m_IsAttachedPortSource)
        throw std::runtime_error("Fixed port and attached port is both input(or both output), can not generate link");

    if (m_IsFixedPortSource)
        return SRenderPassGraphLink{ m_FixedPort , vAttachedPort };
    else
        return SRenderPassGraphLink{ vAttachedPort, m_FixedPort };
}
