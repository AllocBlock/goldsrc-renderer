#include "RenderPassGraphInstance.h"

void CRenderPassGraphInstance::init(vk::CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo, ptr<SSceneInfo> vScene)
{
    m_pDevice = vDevice;
    m_pAppInfo = vAppInfo;
    m_pSceneInfo = vScene;
}

void CRenderPassGraphInstance::updateSceneInfo(ptr<SSceneInfo> vSceneInfo)
{
    for (const auto& Pair : m_PassMap)
        Pair.second->setSceneInfo(vSceneInfo);
}

void CRenderPassGraphInstance::createFromGraph(ptr<SRenderPassGraph> vGraph, CPort::Ptr vSwapchainPort,
    BeforeInitCallback_t vBeforeInitCallback)
{
    if (!vGraph->isValid())
        throw std::runtime_error("Graph is not valid");
    // TODO: keep old pass
    // TODO: create only used pass
    destroy();

    for (const auto& Pair : vGraph->NodeMap)
    {
        size_t NodeId = Pair.first;
        const SRenderPassGraphNode& Node = Pair.second;
        vk::IRenderPass::Ptr pPass = RenderpassLib::createPass(Node.Name);
        if (vBeforeInitCallback)
            vBeforeInitCallback(Node.Name, pPass);
        pPass->init(m_pDevice, m_pAppInfo);
        pPass->setSceneInfo(m_pSceneInfo);
        m_PassMap[NodeId] = pPass;
    }

    for (const auto& Pair : vGraph->LinkMap)
    {
        const SRenderPassGraphLink& Link = Pair.second;
        auto pSrcPass = m_PassMap.at(Link.Source.NodeId);
        auto pDestPass = m_PassMap.at(Link.Destination.NodeId);
        CPortSet::link(pSrcPass->getPortSet(), Link.Source.Name, pDestPass->getPortSet(), Link.Destination.Name);
    }

    vSwapchainPort->unlinkAll();
    auto pEntryPass = m_PassMap.at(vGraph->EntryPortOpt->NodeId);
    CPortSet::link(vSwapchainPort, pEntryPass->getPortSet(), vGraph->EntryPortOpt->Name);

    for (const auto& Pair : m_PassMap)
    {
        _ASSERTE(Pair.second->isValid());
    }
}

void CRenderPassGraphInstance::update(uint32_t vImageIndex) const
{
    for (const auto& Pair : m_PassMap)
        Pair.second->update(vImageIndex);
}

void CRenderPassGraphInstance::renderUI()
{
    for (const auto& Pair : m_PassMap)
        Pair.second->renderUI();
}

void CRenderPassGraphInstance::destroy()
{
    for (const auto& Pair : m_PassMap)
        Pair.second->destroy();
    m_PassMap.clear();
}

vk::IRenderPass::Ptr CRenderPassGraphInstance::getPass(size_t vId) const
{
    return m_PassMap.at(vId);
}
