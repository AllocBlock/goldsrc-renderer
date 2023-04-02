#include "IconManager.h"
#include "StaticResource.h"

const std::map<EIcon, std::filesystem::path> gIconImageFileMap =
{
    {EIcon::TIP, "Icons/Tip.png"}
}; 

ptr<CIconManager> CIconManager::pInstance = nullptr;

CIconManager::CIconManager()
{
    // load icon images
    for (const auto& Pair : gIconImageFileMap)
    {
        m_IconImageMap[Pair.first] = StaticResource::loadImage(Pair.second);
    }
}