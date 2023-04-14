#pragma once
#include "Pointer.h"
#include "RenderPassLib.h"
#include "RenderPassGraph.h"
#include "RenderPass.h"

using BeforeInitCallback_t = std::function<void(const std::string&, vk::IRenderPass::Ptr)>;

class CRenderPassGraphInstance
{
public:
    _DEFINE_PTR(CRenderPassGraphInstance);

    void init(vk::CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo, ptr<SSceneInfo> vScene)
    {
        m_pDevice = vDevice;
        m_pAppInfo = vAppInfo;
        m_pSceneInfo = vScene;
    }

    void createFromGraph(ptr<SRenderPassGraph> vGraph, CPort::Ptr vSwapchainPort, BeforeInitCallback_t vBeforeInitCallback = nullptr)
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

    void update(uint32_t vImageIndex) const
    {
        for (const auto& Pair : m_PassMap)
            Pair.second->update(vImageIndex);
    }

    void setSceneInfo(ptr<SSceneInfo> vSceneInfo)
    {
        for (const auto& Pair : m_PassMap)
            Pair.second->setSceneInfo(vSceneInfo);
    }

    void renderUI()
    {
        for (const auto& Pair : m_PassMap)
            Pair.second->renderUI();
    }

    void destroy()
    {
        for (const auto& Pair : m_PassMap)
            Pair.second->destroy();
        m_PassMap.clear();
    }

    vk::IRenderPass::Ptr getPass(size_t vId)
    {
        return m_PassMap.at(vId);
    }

private:
    vk::CDevice::CPtr m_pDevice = nullptr;
    CAppInfo::Ptr m_pAppInfo = nullptr;
    ptr<SSceneInfo> m_pSceneInfo = nullptr;
    std::map<size_t, vk::IRenderPass::Ptr> m_PassMap;
};