#pragma once
#include "IOImage.h"

#include <vector>
#include <functional>
#include <optional>
#include <glm/glm.hpp>

enum class E3DObjectDataType
{
    TRIAGNLE_LIST,
    TRIAGNLE_STRIP_LIST,
    INDEXED_TRIAGNLE_LIST,
};

// AABB
struct S3DBoundingBox
{
    glm::vec3 Min;
    glm::vec3 Max;
};

enum class E3DObjectRenderType
{
    NORMAL,
    SKY
};

struct S3DObject
{
    E3DObjectDataType DataType = E3DObjectDataType::TRIAGNLE_LIST;
    E3DObjectRenderType RenderType = E3DObjectRenderType::NORMAL;
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec3> Colors;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> TexCoords;
    std::vector<glm::vec2> LightmapCoords;
    std::vector<uint32_t> TexIndices;
    std::vector<uint32_t> Indices;

    bool HasLightmap = false;
    std::vector<std::optional<size_t>> LightmapIndices;

    bool IsTransparent = false;
    float Opacity = 1.0;

    S3DBoundingBox getBoundingBox() const;
};

struct SBspTreeNode
{
    uint32_t Index;
    glm::vec3 PlaneNormal;
    float PlaneDistance;
    std::optional<uint32_t> PvsOffset = std::nullopt;
    std::optional<int32_t> Front = std::nullopt;
    std::optional<int32_t> Back = std::nullopt;

    bool isPointFrontOfPlane(glm::vec3 vPoint) const;
};

struct SBspTree
{
    size_t NodeNum, LeafNum, ModelNum;
    std::vector<SBspTreeNode> Nodes;

    uint32_t getPointLeaf(glm::vec3 vPoint);
};

struct SBspPvs
{
    uint32_t LeafNum;
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
    std::vector<std::shared_ptr<S3DObject>> Objects;
    std::vector<std::shared_ptr<CIOImage>> TexImages;

    bool UseLightmap = false;
    std::shared_ptr<CLightmap> pLightmap = nullptr;

    bool UsePVS = false;
    SBspTree BspTree;
    SBspPvs BspPvs;
};