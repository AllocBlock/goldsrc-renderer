#pragma once
#include "IOImage.h"
#include "Scene.h"

#include <vector>
#include <array>
#include <glm/glm.hpp>

class CLightmap
{
public:
    void clear();
    size_t appendLightmap(sptr<CIOImage> vImage);
    std::pair<size_t, size_t> getLightmapSize();
    glm::vec2 getAcutalLightmapCoord(size_t vIndex, glm::vec2 vOriginTexCoord);
    sptr<CIOImage> getCombinedLightmap();
private:
    const size_t m_MaxWidth = 0x4000;
    std::vector<sptr<CIOImage>> m_LightmapImages;
    size_t m_TotalWidth = 0, m_TotalHeight = 0;
    size_t m_StartHeight = 0, m_StartWidth = 0, m_CurrentRowHeight = 0;
    std::vector<std::pair<size_t, size_t>> m_Offsets;
    static const size_t m_Channel = 4; // RGBA 4 channels
};

enum class EGoldSrcSpriteType
{
    PARALLEL_UP_RIGHT = 0x00, // 始终面向相机，但z轴锁定（比如花草）
    FACING_UP_RIGHT, // 和PARALLEL_UP_RIGHT类似，但是面向方向不由相机朝向决定，而是由玩家位置决定
    PARALLEL, // 始终面向相机（比如光晕）
    ORIENTED, // 固定方向
    PARALLEL_ORIENTED // 始终面向相机，但是法向可以自定义
};

struct SGoldSrcSprite
{
    glm::vec3 Position;
    glm::vec3 Angle;
    float Scale;
    EGoldSrcSpriteType Type;
    sptr<CIOImage> pImage;
};

struct SSceneInfo
{
    const sptr<CScene> pScene = make<CScene>();
    std::vector<sptr<CIOImage>> TexImageSet;
    std::vector<SGoldSrcSprite> SprSet;

    bool UseSkyBox = false;
    std::array<sptr<CIOImage>, 6> SkyBoxImages;

    // for bsp
    bool UseLightmap = false;
    sptr<CLightmap> pLightmap = nullptr;

    void clear()
    {
        pScene->clear();
        TexImageSet.clear();
        SprSet.clear();
        UseSkyBox = false;
        SkyBoxImages.fill(nullptr);
        UseLightmap = false;
        pLightmap = nullptr;
    }
};
