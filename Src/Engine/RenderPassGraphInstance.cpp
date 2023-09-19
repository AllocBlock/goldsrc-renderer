#include "RenderPassGraphInstance.h"

#include <queue>

template <typename T>
size_t getIndex(const std::vector<T>& vSet, const T& vTarget)
{
    for (size_t i = 0; i < vSet.size(); ++i)
        if (vSet[i] == vTarget)
            return i;
    throw std::runtime_error("Port set not found, which is unexpected situation");
}

struct SPortGroup
{
    std::vector<CPort::Ptr> Ports;

    bool hasPort(CPort::Ptr vPort)
    {
        return std::find(Ports.begin(), Ports.end(), vPort) != Ports.end();
    }

    void add(CPort::Ptr vPort)
    {
        if (!hasPort(vPort))
            Ports.push_back(vPort);
    }

    void sort(const std::vector<CPortSet::Ptr>& vSortedPortSets)
    {
        std::vector<CPortSet*> SortedPortSets(vSortedPortSets.size());
        for (size_t i = 0; i < SortedPortSets.size(); ++i)
            SortedPortSets[i] = vSortedPortSets[i].get();
        auto Comparer = [&SortedPortSets](const CPort::Ptr& vPort1, const CPort::Ptr& vPort2) -> bool
        {
            size_t i1 = std::find(SortedPortSets.begin(), SortedPortSets.end(), vPort1->getBelongedPortSet()) - SortedPortSets.begin();
            size_t i2 = std::find(SortedPortSets.begin(), SortedPortSets.end(), vPort2->getBelongedPortSet()) - SortedPortSets.begin();
            return i1 < i2;
        };
        std::sort(Ports.begin(), Ports.end(), Comparer);
    }
};

std::vector<SPortGroup> __getSortedPortGroups(const std::vector<CPortSet::Ptr>& vSortedPortSets)
{
    std::vector<SPortGroup> PortGroups;

    for (const auto& pPortSet : vSortedPortSets)
    {
        std::vector<CPort::Ptr> Ports;
        for (size_t i = 0; i < pPortSet->getInputPortNum(); ++i)
            Ports.push_back(pPortSet->getInputPort(i));
        for (size_t i = 0; i < pPortSet->getOutputPortNum(); ++i)
            Ports.push_back(pPortSet->getOutputPort(i));

        for (const auto& pPort : Ports)
        {
            bool FoundExistGroup = false;
            if (pPort->hasParent())
            {
                CPort::Ptr pParent = pPort->getParent();
                for (auto& Group : PortGroups)
                {
                    if (Group.hasPort(pParent))
                    {
                        Group.add(pPort);
                        FoundExistGroup = true;
                        break;
                    }
                }
            }
            if (!FoundExistGroup)
            {
                SPortGroup PortGroup;
                PortGroup.add(pPort);
                PortGroups.emplace_back(PortGroup);
            }
        }
    }

    for (auto& Group : PortGroups)
    {
        Group.sort(vSortedPortSets);
    }

    return PortGroups;
}

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

void CRenderPassGraphInstance::createFromGraph(ptr<SRenderPassGraph> vGraph, size_t vPresentNodeId, BeforeInitCallback_t vBeforeInitCallback)
{
    if (!vGraph->isValid())
        throw std::runtime_error("Graph is not valid");
    destroy();

    // remove unused renderpass
    std::map<size_t, std::set<size_t>> DependPass;
    for (const auto& Pair : vGraph->NodeMap)
        DependPass[Pair.first] = {};

    for (const auto& Pair : vGraph->LinkMap)
    {
        const SRenderPassGraphLink& Link = Pair.second;
        DependPass.at(Link.Destination.NodeId).insert(Link.Source.NodeId);
    }

    std::set<size_t> UsedRenderpass;
    std::queue<size_t> Queue;
    Queue.push(vPresentNodeId);
    while(!Queue.empty())
    {
        size_t NodeId = Queue.front(); Queue.pop();
        UsedRenderpass.insert(NodeId);
        for (size_t DependNodeId : DependPass.at(NodeId))
        {
            if (!UsedRenderpass.count(DependNodeId))
                Queue.push(DependNodeId);
        }
    }

    // summarize depend info
    std::map<size_t, size_t> RemainDependNumMap;
    std::map<size_t, std::set<size_t>> DependPassSet;
    for (const auto& Pair : vGraph->NodeMap)
    {
        size_t NodeId = Pair.first;
        if (!UsedRenderpass.count(NodeId)) continue;

        const SRenderPassGraphNode& Node = Pair.second;
        vk::IRenderPass::Ptr pPass = RenderpassLib::createPass(Node.Name);
        if (vBeforeInitCallback)
            vBeforeInitCallback(Node.Name, pPass);
        pPass->createPortSet();
        m_PassMap[NodeId] = pPass;
        
        RemainDependNumMap[NodeId] = 0;
        DependPassSet[NodeId] = {};
    }

    for (const auto& Pair : vGraph->LinkMap)
    {
        const SRenderPassGraphLink& Link = Pair.second;
        size_t SrcNodeId = Link.Source.NodeId;
        size_t DstNodeId = Link.Destination.NodeId;
        if (!UsedRenderpass.count(SrcNodeId) || !UsedRenderpass.count(DstNodeId)) continue;
        auto pSrcPass = m_PassMap.at(SrcNodeId);
        auto pDestPass = m_PassMap.at(DstNodeId);
        CPortSet::link(pSrcPass->getPortSet(), Link.Source.Name, pDestPass->getPortSet(), Link.Destination.Name);

        auto& DependNodeIdSet = DependPassSet.at(DstNodeId);
        if (DependNodeIdSet.find(SrcNodeId) == DependNodeIdSet.end())
        {
            DependNodeIdSet.insert(SrcNodeId);
            RemainDependNumMap.at(SrcNodeId)++;
        }
    }

    // sort
    std::queue<size_t> Leaves;
    Leaves.push(vPresentNodeId);

    while (!Leaves.empty())
    {
        size_t NodeId = Leaves.front(); Leaves.pop();
        m_SortedOrder.push_back(NodeId);
        for (size_t DependNodeId : DependPassSet.at(NodeId))
        {
            if (RemainDependNumMap.at(DependNodeId) == 1)
                Leaves.push(DependNodeId);
            RemainDependNumMap[DependNodeId]--;
        }
    }
    std::reverse(m_SortedOrder.begin(), m_SortedOrder.end());

    // set port layout
    std::vector<CPortSet::Ptr> SortedPortSets;
    for (size_t NodeId : m_SortedOrder)
        SortedPortSets.push_back(m_PassMap.at(NodeId)->getPortSet());

    std::vector<SPortGroup> PortGroups = __getSortedPortGroups(SortedPortSets);
    for (const auto& Group : PortGroups)
    {
        for (size_t i = 0; i < Group.Ports.size() - 1; ++i)
        {
            VkImageLayout Layout = Group.Ports[i + 1]->getInputLayout();
            _ASSERTE(Layout != VK_IMAGE_LAYOUT_UNDEFINED);
            Group.Ports[i]->setOutputLayout(Layout);
        }
    }

    for (const auto& Group : PortGroups)
    {
        VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (Group.Ports.size() >= 2)
            FinalLayout = Group.Ports.back()->getInputLayout();
        Group.Ports.back()->setOutputLayout(FinalLayout);
    }
    
    for (const auto& Pair : m_PassMap)
    {
        auto pPortSet = Pair.second->getPortSet();

        for (size_t i = 0; i < pPortSet->getInputPortNum(); ++i)
        {
            const auto& pPort = pPortSet->getInputPort(i);
            _ASSERTE(pPort->hasInputLayout());
            _ASSERTE(pPort->hasOutputLayout());
        }
        for (size_t i = 0; i < pPortSet->getOutputPortNum(); ++i)
        {
            const auto& pPort = pPortSet->getOutputPort(i);
            _ASSERTE(pPort->hasInputLayout());
            _ASSERTE(pPort->hasOutputLayout());
        }
    }
    
    // init
     for (const auto& Pair : m_PassMap)
    {
        vk::IRenderPass::Ptr pPass = Pair.second;
        pPass->init(m_pDevice, m_ImageNum, m_ScreenExtent);
        pPass->setSceneInfo(m_pSceneInfo);
    }

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
