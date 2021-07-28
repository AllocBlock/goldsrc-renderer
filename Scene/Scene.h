#pragma once
#include "IOImage.h"
#include "3DObjectGoldSrc.h"

#include <vector>
#include <functional>
#include <array>
#include <map>
#include <glm/glm.hpp>

struct SModelInfo
{
    S3DBoundingBox BoundingBox;
    bool IsTransparent = false;
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
    size_t appendLightmap(std::shared_ptr<CIOImage> vpImage);
    std::pair<size_t, size_t> getLightmapSize();
    glm::vec2 getAcutalLightmapCoord(size_t vIndex, glm::vec2 vOriginTexCoord);
    std::shared_ptr<CIOImage> getCombinedLightmap();
private:
    const size_t m_MaxWidth = 0x4000;
    std::vector<std::shared_ptr<CIOImage>> m_LightmapImages;
    size_t m_TotalWidth = 0, m_TotalHeight = 0;
    size_t m_StartHeight = 0, m_StartWidth = 0, m_CurrentRowHeight = 0;
    std::vector<std::pair<size_t, size_t>> m_Offsets;
    static const size_t m_Channel = 4; // RGBA 4 channels
};

struct SScene
{
    std::vector<std::shared_ptr<C3DObjectGoldSrc>> Objects;
    std::vector<std::shared_ptr<CIOImage>> TexImageSet;

    bool UseLightmap = false;
    std::shared_ptr<CLightmap> pLightmap = nullptr;

    bool UsePVS = false;
    SBspTree BspTree;
    SBspPvs BspPvs;

    bool UseSkyBox = false;
    std::array<std::shared_ptr<CIOImage>, 6> SkyBoxImages;
};