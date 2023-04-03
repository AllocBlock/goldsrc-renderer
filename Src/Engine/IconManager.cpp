#include "IconManager.h"
#include "StaticResource.h"

struct SIconSource
{
    EIcon Icon;
    std::filesystem::path Path;
    EIconRenderType RenderType;
};

const std::vector<SIconSource> gIconImageFileMap =
{
    {EIcon::TIP, "Icons/Tip.png", EIconRenderType::INDEXED_TRANSPARENT},
    {EIcon::QUESTION, "Icons/Question.png", EIconRenderType::INDEXED_TRANSPARENT},
    {EIcon::ALERT, "Icons/Alert.png", EIconRenderType::INDEXED_TRANSPARENT},
}; 

ptr<CIconManager> CIconManager::pInstance = nullptr;

CIconManager::CIconManager()
{
    // load icon images
    for (const auto& IconSource : gIconImageFileMap)
    {
        m_IconImageMap[IconSource.Icon] = { StaticResource::loadImage(IconSource.Path), IconSource.RenderType };
    }
}