#pragma once
#include "IOImage.h"
#include "MeshDataGoldSrc.h"
#include "TempScene.h"
#include "BoundingBox.h"

#include <vector>
#include <array>
#include <map>
#include <optional>
#include <glm/glm.hpp>

enum class EGoldSrcRenderMode
{
    NORMAL = 0x00,
    COLOR,
    TEXTURE,
    GLOW,
    SOLID,
    ADDITIVE
};

struct SModelInfo
{
    SAABB BoundingBox;
    EGoldSrcRenderMode RenderMode = EGoldSrcRenderMode::NORMAL;
    float Opacity = 1.0f;
};

struct SBspTreeNode
{
    glm::vec3 PlaneNormal;
    float PlaneDistance = 0.0f;
    std::optional<uint32_t> PvsOffset = std::nullopt;
    std::optional<int32_t> Front = std::nullopt;
    std::optional<int32_t> Back = std::nullopt;

    bool isPointFrontOfPlane(glm::vec3 vPoint) const;
};

struct SBspTree
{
    size_t NodeNum = 0, LeafNum = 0, ModelNum = 0;
    std::vector<SBspTreeNode> Nodes;
    std::map<size_t, std::vector<size_t>> LeafIndexToObjectIndices;
    std::map<size_t, std::vector<size_t>> ModelIndexToObjectIndex;
    std::vector<SModelInfo> ModelInfos;

    uint32_t getPointLeaf(glm::vec3 vPoint);
};

struct SBspPvs
{
    uint32_t LeafNum = 0;
    std::vector<uint8_t> RawData;
    std::vector<std::vector<bool>> MapList;

    void decompress(std::vector<uint8_t> vRawData, const SBspTree& vBspTree);
    bool isLeafVisiable(uint32_t vStartLeafIndex, uint32_t vLeafIndex) const;
private:
    std::vector<uint8_t> __decompressFrom(size_t vStartIndex);
};

class CLightmap
{
public:
    void clear();
    size_t appendLightmap(ptr<CIOImage> vpImage);
    std::pair<size_t, size_t> getLightmapSize();
    glm::vec2 getAcutalLightmapCoord(size_t vIndex, glm::vec2 vOriginTexCoord);
    ptr<CIOImage> getCombinedLightmap();
private:
    const size_t m_MaxWidth = 0x4000;
    std::vector<ptr<CIOImage>> m_LightmapImages;
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
    ptr<CIOImage> pImage;
};

struct SSceneInfoGoldSrc
{
    CTempScene<CMeshDataGoldSrc>::Ptr pScene = nullptr;
    std::vector<ptr<CIOImage>> TexImageSet;
    std::vector<SGoldSrcSprite> SprSet;

    // for bsp
    bool UseLightmap = false;
    ptr<CLightmap> pLightmap = nullptr;

    // for bsp
    bool UsePVS = false;
    SBspTree BspTree;
    SBspPvs BspPvs;

    bool UseSkyBox = false;
    std::array<ptr<CIOImage>, 6> SkyBoxImages;
};
