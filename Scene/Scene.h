#pragma once
#include "IOImage.h"

#include <vector>
#include <functional>
#include <optional>
#include <glm/glm.hpp>

enum class E3DObjectType
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

struct S3DObject
{
    bool UseShadow = true;
    bool UseLightmap = false;

    E3DObjectType Type = E3DObjectType::TRIAGNLE_LIST;
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec3> Colors;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> TexCoords;
    std::vector<glm::vec2> LightmapCoords;
    std::vector<uint32_t> TexIndices;
    std::vector<uint32_t> LightmapIndices;
    std::vector<uint32_t> Indices;

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
};

struct SBspTree
{
    size_t NodeNum, LeafNum;
    std::vector<SBspTreeNode> Nodes;

    uint32_t getPointLeaf(glm::vec3 vPoint);
};

struct SBspPvs
{
    uint32_t LeafNum;
    std::vector<uint8_t> RawData;
    std::vector<std::vector<bool>> MapList;

    void decompress(std::vector<uint8_t> vRawData, const SBspTree& vBspTree);
    bool isVisiableLeafVisiable(uint32_t vStartLeafIndex, uint32_t vLeafIndex) const;
private:
    std::vector<uint8_t> __decompressFrom(size_t vStartIndex);
};

struct SScene
{
    bool UseLightmap = false;

    std::vector<std::shared_ptr<S3DObject>> Objects;
    std::vector<std::shared_ptr<CIOImage>> TexImages;
    std::vector<std::shared_ptr<CIOImage>> LightmapImages;

    bool UsePVS = false;
    SBspTree BspTree;
    SBspPvs BspPvs;
};

namespace SceneReader
{
    SScene readBspFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
    SScene readMapFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
    SScene readObjFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
}