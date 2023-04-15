#include "RenderpassLib.h"
#include "VectorMap.h"

#include <functional>
#include <string>
#include <vector>
#include <typeindex>

CVectorMap<std::string, RenderpassLib::CreateRenderPassFunc_t> gNamePassCreatorVecMap = {};
std::unordered_map<std::type_index, std::string> gTypePassNameMap = {};

void RenderpassLib::registerRenderPass(const std::string& vName, const type_info& vTypeId, CreateRenderPassFunc_t vCreateFunction)
{
    if (gNamePassCreatorVecMap.has(vName))
        throw std::runtime_error("Pass already registered, or duplicate name occurs");
    if (gTypePassNameMap.find(vTypeId) != gTypePassNameMap.end())
        throw std::runtime_error("Pass already registered, or duplicate name occurs");
    _ASSERTE(vCreateFunction);
    gNamePassCreatorVecMap.set(vName, vCreateFunction);
    gTypePassNameMap[vTypeId] = vName;
}

bool RenderpassLib::hasPass(const std::string& vName)
{
    return gNamePassCreatorVecMap.has(vName);
}

std::vector<std::string> RenderpassLib::getAllPassNames()
{
    return gNamePassCreatorVecMap.getKeys();
}

vk::IRenderPass::Ptr RenderpassLib::createPass(const std::string& vName)
{
    if (!gNamePassCreatorVecMap.has(vName))
        throw std::runtime_error("Pass not found, register it before create it by sign");
    return gNamePassCreatorVecMap.get(vName)();
}

const std::string& RenderpassLib::getPassName(const vk::IRenderPass::Ptr& vPass)
{
    _ASSERTE(vPass);
    const auto& TypeIndex = std::type_index(typeid(*vPass));
    if (gTypePassNameMap.find(TypeIndex) == gTypePassNameMap.end())
        throw std::runtime_error("Pass not found, register it before create it by sign");
    return gTypePassNameMap.at(TypeIndex);
}