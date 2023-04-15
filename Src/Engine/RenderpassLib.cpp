#include "RenderpassLib.h"

#include <functional>
#include <string>
#include <vector>
#include <typeindex>

std::map<std::string, RenderpassLib::CreateRenderPassFunc_t> gNamePassCreatorMap = {};
std::map<std::type_index, std::string> gTypePassNameMap = {};

void RenderpassLib::registerRenderPass(const std::string& vName, const type_info& vTypeId, CreateRenderPassFunc_t vCreateFunction)
{
    if (gNamePassCreatorMap.find(vName) != gNamePassCreatorMap.end())
        throw std::runtime_error("Pass already registered, or duplicate name occurs");
    if (gTypePassNameMap.find(vTypeId) != gTypePassNameMap.end())
        throw std::runtime_error("Pass already registered, or duplicate name occurs");
    _ASSERTE(vCreateFunction);
    gNamePassCreatorMap[vName] = vCreateFunction;
    gTypePassNameMap[vTypeId] = vName;
}

bool RenderpassLib::hasPass(const std::string& vName)
{
    return gNamePassCreatorMap.find(vName) != gNamePassCreatorMap.end();
}

std::vector<std::string> RenderpassLib::getAllPassNames()
{
    std::vector<std::string> NameSet;
    for (const auto& Pair : gNamePassCreatorMap)
        NameSet.push_back(Pair.first);
    return NameSet;
}

vk::IRenderPass::Ptr RenderpassLib::createPass(const std::string& vName)
{
    if (gNamePassCreatorMap.find(vName) == gNamePassCreatorMap.end())
        throw std::runtime_error("Pass not found, register it before create it by sign");
    return gNamePassCreatorMap.at(vName)();
}

const std::string& RenderpassLib::getPassName(const vk::IRenderPass::Ptr& vPass)
{
    _ASSERTE(vPass);
    const auto& TypeIndex = std::type_index(typeid(*vPass));
    if (gTypePassNameMap.find(TypeIndex) == gTypePassNameMap.end())
        throw std::runtime_error("Pass not found, register it before create it by sign");
    return gTypePassNameMap.at(TypeIndex);
}