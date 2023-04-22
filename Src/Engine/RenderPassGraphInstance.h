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

    void init(vk::CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo, ptr<SSceneInfo> vScene);
    void updateSceneInfo(ptr<SSceneInfo> vSceneInfo);
    void createFromGraph(ptr<SRenderPassGraph> vGraph, CPort::Ptr vSwapchainPort, BeforeInitCallback_t vBeforeInitCallback = nullptr);
    void update(uint32_t vImageIndex) const;
    void renderUI();
    void destroy();

    // return pass by given id
    vk::IRenderPass::Ptr getPass(size_t vId) const;

    // return first found pass of given type (by pass name), return nullptr if not found
    template <typename RenderPass_t>
    ptr<RenderPass_t> findPass() const
    {
        for (const auto& Pair : m_PassMap)
            if (RenderpassLib::getPassName(Pair.second) == RenderPass_t::Name)
                return std::dynamic_pointer_cast<RenderPass_t>(Pair.second);
        return nullptr;
    }

private:
    vk::CDevice::CPtr m_pDevice = nullptr;
    CAppInfo::Ptr m_pAppInfo = nullptr;
    ptr<SSceneInfo> m_pSceneInfo = nullptr;
    std::map<size_t, vk::IRenderPass::Ptr> m_PassMap;
};