#pragma once

template <typename RenderPass_t>
class CRenderPassRegister
{
public:
    CRenderPassRegister(const std::string& vName)
    {
        CRenderPassGraphUI::registerRenderPass(vName, []() -> vk::IRenderPass::Ptr { return make<RenderPass_t>(); });
    }
};