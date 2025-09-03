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
    
    void init(cptr<vk::CDevice> vDevice, VkExtent2D vScreenExtent, sptr<SSceneInfo> vScene);
    void updateSceneInfo(sptr<SSceneInfo> vSceneInfo);
    void createFromGraph(sptr<SRenderPassGraph> vGraph, GLFWwindow* vpWindow, wptr<vk::CSwapchain> vpSwapchain);
    void update() const;
    void renderUI();
    void destroy();

    void updateSwapchainImageIndex(uint32_t vImageIndex);

    // return pass by given id
    sptr<engine::IRenderPass> getPass(size_t vId) const;

    // return first found pass of given type (by pass name), return nullptr if not found
    template <typename RenderPass_t>
    sptr<RenderPass_t> findPass() const
    {
        for (const auto& Pair : m_PassMap)
            if (RenderpassLib::getPassName(Pair.second) == RenderPass_t::Name)
                return std::dynamic_pointer_cast<RenderPass_t>(Pair.second);
        return nullptr;
    }

    std::vector<sptr<engine::IRenderPass>> getSortedPasses() const
    {
        std::vector<sptr<engine::IRenderPass>> Passes;
        for (auto PassId : m_SortedOrder)
        {
            Passes.push_back(m_PassMap.at(PassId));
        }
        return Passes;
    }

private:
    std::vector<size_t> m_SortedOrder;
    cptr<vk::CDevice> m_pDevice = nullptr;
    VkExtent2D m_ScreenExtent = vk::ZeroExtent;
    sptr<SSceneInfo> m_pSceneInfo = nullptr;
    std::map<size_t, sptr<engine::IRenderPass>> m_PassMap;

    sptr<CRenderPassGUI> m_pPassGui;
    sptr<CRenderPassPresent> m_pPassPresent;
};