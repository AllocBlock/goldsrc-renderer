#pragma once
#include "RenderPass.h"

namespace RenderpassLib
{
    using CreateRenderPassFunc_t = std::function<vk::IRenderPass::Ptr()>;
    void registerRenderPass(const std::string& vName, CreateRenderPassFunc_t vCreateFunction);

    bool hasPass(const std::string& vName);
    std::vector<std::string> getAllPassNames();
    vk::IRenderPass::Ptr createPass(const std::string& vName);
}

template <typename RenderPass_t>
class CRenderPassRegister
{
public:
    CRenderPassRegister(const std::string& vName)
    {
        RenderpassLib::registerRenderPass(vName, []() -> vk::IRenderPass::Ptr { return make<RenderPass_t>(); });
    }
};