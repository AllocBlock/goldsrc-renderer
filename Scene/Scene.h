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

    S3DBoundingBox getBoundingBox();
private:
    std::optional<S3DBoundingBox> m_BoundingBox = std::nullopt;
};

struct SScene
{
    bool UseLightmap = false;

    std::vector<std::shared_ptr<S3DObject>> Objects;
    std::vector<std::shared_ptr<CIOImage>> TexImages;
    std::vector<std::shared_ptr<CIOImage>> LightmapImages;
};

namespace SceneReader
{
    SScene readBspFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
    SScene readMapFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
    SScene readObjFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
}