#pragma once
#include "RenderPass.h"

namespace RenderpassLib
{
    using CreateRenderPassFunc_t = std::function<vk::IRenderPass::Ptr()>;
    void registerRenderPass(const std::string& vName, const type_info& vTypeId, bool vVisible, CreateRenderPassFunc_t vCreateFunction);

    bool hasPass(const std::string& vName);
    const std::vector<std::string>& getAllPassNames();
    const std::vector<std::string>& getAllVisiblePassNames();
    vk::IRenderPass::Ptr createPass(const std::string& vName);
    const std::string& getPassName(const vk::IRenderPass::Ptr& vPass);
}

template <typename RenderPass_t, bool Visible>
class CRenderPassRegister
{
public:
    CRenderPassRegister()
    {
        RenderpassLib::registerRenderPass(RenderPass_t::Name, typeid(RenderPass_t), Visible, []() -> vk::IRenderPass::Ptr { return make<RenderPass_t>(); });
    }
};