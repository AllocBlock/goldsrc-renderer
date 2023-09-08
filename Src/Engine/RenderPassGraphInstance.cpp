#include "RenderPassGraphInstance.h"

#include <queue>

void CRenderPassGraphInstance::init(vk::CDevice::CPtr vDevice, size_t vImageNum, VkExtent2D vScreenExtent, ptr<SSceneInfo> vScene)
{
    _ASSERTE(vImageNum > 0);
    _ASSERTE(vScreenExtent.width > 0 && vScreenExtent.height > 0);
    m_pDevice = vDevice;
    m_ImageNum = vImageNum;
    m_ScreenExtent = vScreenExtent;

    m_pSceneInfo = vScene;
}

void CRenderPassGraphInstance::updateSceneInfo(ptr<SSceneInfo> vSceneInfo)
{
    for (const auto& Pair : m_PassMap)
        Pair.second->setSceneInfo(vSceneInfo);
}

void CRenderPassGraphInstance::createFromGraph(ptr<SRenderPassGraph> vGraph, BeforeInitCallback_t vBeforeInitCallback)
{
    if (!vGraph->isValid())
        throw std::runtime_error("Graph is not valid");
    destroy();

    std::map<size_t, int> RemainDependNumSet;
    std::map<size_t, std::vector<size_t>> DependPassSet;
    for (const auto& Pair : vGraph->NodeMap)
    {
        size_t NodeId = Pair.first;
        const SRenderPassGraphNode& Node = Pair.second;
        vk::IRenderPass::Ptr pPass = RenderpassLib::createPass(Node.Name);
        if (vBeforeInitCallback)
            vBeforeInitCallback(Node.Name, pPass);
        pPass->createPortSet();
        m_PassMap[NodeId] = pPass;

        RemainDependNumSet[NodeId] = 0;
        DependPassSet[NodeId] = {};
    }

    for (const auto& Pair : vGraph->LinkMap)
    {
        const SRenderPassGraphLink& Link = Pair.second;
        auto pSrcPass = m_PassMap.at(Link.Source.NodeId);
        auto pDestPass = m_PassMap.at(Link.Destination.NodeId);
        CPortSet::link(pSrcPass->getPortSet(), Link.Source.Name, pDestPass->getPortSet(), Link.Destination.Name);

        DependPassSet.at(Link.Destination.NodeId).push_back(Link.Source.NodeId);
    }

    for (const auto& Pair : m_PassMap)
    {
        vk::IRenderPass::Ptr pPass = Pair.second;
        pPass->init(m_pDevice, m_ImageNum, m_ScreenExtent);
        pPass->setSceneInfo(m_pSceneInfo);
    }

    // sort
    std::queue<size_t> Leaves;
    for (const auto& Pair : RemainDependNumSet)
    {
        if (Pair.second == 0)
            Leaves.push(Pair.first);
    }

    while (!Leaves.empty())
    {
        size_t NodeId = Leaves.front(); Leaves.pop();
        if (!m_PassMap.at(NodeId)->getPortSet()->isInputLinkReady())
            continue;
        m_SortedOrder.push_back(NodeId);
        for (size_t DependNodeId : DependPassSet.at(NodeId))
        {
            if (RemainDependNumSet.at(DependNodeId) == 1)
                Leaves.push(DependNodeId);
            RemainDependNumSet[DependNodeId]--;
        }
    }
    std::reverse(m_SortedOrder.begin(), m_SortedOrder.end());

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
    m_SortedOrder.clear();
}

vk::IRenderPass::Ptr CRenderPassGraphInstance::getPass(size_t vId) const
{
    return m_PassMap.at(vId);
}
