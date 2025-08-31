#pragma once
#include "Pointer.h"
#include "RenderPassLib.h"
#include "RenderPassGraph.h"
#include "RenderPass.h"
#include "PassGUI.h"
#include "PassPresent.h"

class CRenderPassGraphInstance
{
public:
    _DEFINE_PTR(CRenderPassGraphInstance);

    void init(vk::CDevice::CPtr vDevice, VkExtent2D vScreenExtent, ptr<SSceneInfo> vScene);
    void updateSceneInfo(ptr<SSceneInfo> vSceneInfo);
    void createFromGraph(ptr<SRenderPassGraph> vGraph, GLFWwindow* vpWindow, wptr<vk::CSwapchain> vpSwapchain);
    void update() const;
    void renderUI();
    void destroy();

    void updateSwapchainImageIndex(uint32_t vImageIndex);

    // return pass by given id
    engine::IRenderPass::Ptr getPass(size_t vId) const;

    // return first found pass of given type (by pass name), return nullptr if not found
    template <typename RenderPass_t>
    ptr<RenderPass_t> findPass() const
    {
        for (const auto& Pair : m_PassMap)
            if (RenderpassLib::getPassName(Pair.second) == RenderPass_t::Name)
                return std::dynamic_pointer_cast<RenderPass_t>(Pair.second);
        return nullptr;
    }

    std::vector<engine::IRenderPass::Ptr> getSortedPasses() const
    {
        std::vector<engine::IRenderPass::Ptr> Passes;
        for (auto PassId : m_SortedOrder)
        {
            Passes.push_back(m_PassMap.at(PassId));
        }
        return Passes;
    }

private:
    std::vector<size_t> m_SortedOrder;
    vk::CDevice::CPtr m_pDevice = nullptr;
    VkExtent2D m_ScreenExtent = vk::ZeroExtent;
    ptr<SSceneInfo> m_pSceneInfo = nullptr;
    std::map<size_t, engine::IRenderPass::Ptr> m_PassMap;

    ptr<CRenderPassGUI> m_pPassGui;
    ptr<CRenderPassPresent> m_pPassPresent;
};