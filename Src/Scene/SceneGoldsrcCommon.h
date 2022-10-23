#pragma once
#include "IOGoldsrcWad.h"
#include "MeshDataGoldSrc.h"
#include "Actor.h"

#include <vector>
#include <filesystem>

namespace GoldSrc
{
    CActor<CMeshDataGoldSrc>::Ptr createActorByMeshAndTag(const CMeshDataGoldSrc& vMeshData, const std::vector<std::string> vTagSet = {});
    bool readWad(std::filesystem::path vWadPath, std::filesystem::path vAdditionalSearchDir, CIOGoldsrcWad& voWad);
    std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths, std::filesystem::path vAdditionalSearchDir = "");
}
