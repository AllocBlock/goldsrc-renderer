#include "RenderpassLib.h"

#include <functional>
#include <string>
#include <vector>

std::map<std::string, RenderpassLib::CreateRenderPassFunc_t> gRegisteredPassSet = {};

void RenderpassLib::registerRenderPass(const std::string& vName, CreateRenderPassFunc_t vCreateFunction)
{
    if (gRegisteredPassSet.find(vName) != gRegisteredPassSet.end())
        throw std::runtime_error("Pass already registered, or duplicate name occurs");
    _ASSERTE(vCreateFunction);
    gRegisteredPassSet[vName] = vCreateFunction;
}

bool RenderpassLib::hasPass(const std::string& vName)
{
    return gRegisteredPassSet.find(vName) != gRegisteredPassSet.end();
}

std::vector<std::string> RenderpassLib::getAllPassNames()
{
    std::vector<std::string> NameSet;
    for (const auto& Pair : gRegisteredPassSet)
        NameSet.push_back(Pair.first);
    return NameSet;
}

vk::IRenderPass::Ptr RenderpassLib::createPass(const std::string& vName)
{
    if (gRegisteredPassSet.find(vName) == gRegisteredPassSet.end())
        throw std::runtime_error("Pass not found, register it before create it by sign");
    return gRegisteredPassSet.at(vName)();
}