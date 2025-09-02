#include "RenderpassLib.h"
#include "VectorMap.h"
#include "PassGui.h"
#include "PassPresent.h"

#include <functional>
#include <string>
#include <vector>
#include <typeindex>

CVectorMap<std::string, RenderpassLib::CreateRenderPassFunc_t> gNamePassCreatorVecMap = {};
std::vector<std::string> gPassNameSet = {};
std::vector<std::string> gVisiblePassNameSet = {};
std::unordered_map<std::type_index, std::string> gTypePassNameMap = {};

void RenderpassLib::registerRenderPass(const std::string& vName, const type_info& vTypeId, bool vVisible, CreateRenderPassFunc_t vCreateFunction)
{
    if (gNamePassCreatorVecMap.has(vName))
        throw std::runtime_error("Pass already registered, or duplicate name occurs");
    if (gTypePassNameMap.find(vTypeId) != gTypePassNameMap.end())
        throw std::runtime_error("Pass already registered, or duplicate name occurs");
    _ASSERTE(vCreateFunction);
    gNamePassCreatorVecMap.set(vName, vCreateFunction);
    gTypePassNameMap[vTypeId] = vName;
    gPassNameSet.push_back(vName);
    if (vVisible)
        gVisiblePassNameSet.push_back(vName);
}

bool RenderpassLib::hasPass(const std::string& vName)
{
    return gNamePassCreatorVecMap.has(vName);
}

const std::vector<std::string>& RenderpassLib::getAllPassNames()
{
    return gPassNameSet;
}

const std::vector<std::string>& RenderpassLib::getAllVisiblePassNames()
{
    return gVisiblePassNameSet;
}

engine::IRenderPass::Ptr RenderpassLib::createPass(const std::string& vName)
{
    if (!gNamePassCreatorVecMap.has(vName))
        throw std::runtime_error("Pass not found, register it before create it by sign");
    return gNamePassCreatorVecMap.get(vName)();
}

const std::string& RenderpassLib::getPassName(const engine::IRenderPass::Ptr& vPass)
{
    _ASSERTE(vPass);
    if (dynamic_cast<CRenderPassGUI*>(vPass.get()))
    {
        return "Gui";
    }
    else if (dynamic_cast<CRenderPassPresent*>(vPass.get()))
    {
        return "Present";
    }
    const auto& TypeIndex = std::type_index(typeid(*vPass));
    if (gTypePassNameMap.find(TypeIndex) == gTypePassNameMap.end())
        throw std::runtime_error("Pass not found, register it before create it by sign");
    return gTypePassNameMap.at(TypeIndex);
}