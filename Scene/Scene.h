#pragma once
#include "IOImage.h"

#include <vector>
#include <functional>
#include <glm/glm.hpp>

enum class E3DObjectType
{
    TRIAGNLES_LIST,
    INDEXED_TRIAGNLES_LIST
};

struct S3DObject
{
    E3DObjectType Type = E3DObjectType::TRIAGNLES_LIST;
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec3> Colors;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> TexCoords;
    std::vector<uint32_t> Indices;
    uint32_t TexIndex;
};

struct SScene
{
    std::vector<std::shared_ptr<S3DObject>> Objects;
    std::vector<std::shared_ptr<CIOImage>> TexImages;
};

namespace SceneReader
{
    SScene readMapFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
    SScene readObjFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
}