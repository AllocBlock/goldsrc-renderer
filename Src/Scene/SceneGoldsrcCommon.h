#pragma once
#include "IOGoldsrcWad.h"
#include "Actor.h"
#include "Mesh.h"

#include <vector>
#include <filesystem>

namespace GoldSrc
{
    CActor::Ptr createActorByMeshAndTag(const CMeshData& vMeshData, const std::vector<std::string> vTagSet = {});
    bool readWad(std::filesystem::path vWadPath, std::filesystem::path vAdditionalSearchDir, CIOGoldsrcWad& voWad);
    std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths, std::filesystem::path vAdditionalSearchDir = "");
}
