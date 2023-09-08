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

    void init(vk::CDevice::CPtr vDevice, size_t vImageNum, VkExtent2D vScreenExtent, ptr<SSceneInfo> vScene);
    void updateSceneInfo(ptr<SSceneInfo> vSceneInfo);
    void createFromGraph(ptr<SRenderPassGraph> vGraph, BeforeInitCallback_t vBeforeInitCallback = nullptr);
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

    std::vector<vk::IRenderPass::Ptr> getSortedPasses() const
    {
        std::vector<vk::IRenderPass::Ptr> Passes;
        for (auto PassId : m_SortedOrder)
        {
            Passes.push_back(m_PassMap.at(PassId));
        }
        return Passes;
    }

private:
    std::vector<size_t> m_SortedOrder;
    vk::CDevice::CPtr m_pDevice = nullptr;
    size_t m_ImageNum = 0;
    VkExtent2D m_ScreenExtent = vk::ZeroExtent;
    ptr<SSceneInfo> m_pSceneInfo = nullptr;
    std::map<size_t, vk::IRenderPass::Ptr> m_PassMap;
};