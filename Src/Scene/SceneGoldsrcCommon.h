#pragma once
#include "IOGoldsrcWad.h"

#include <vector>
#include <filesystem>

namespace GoldSrc
{
    bool readWad(std::filesystem::path vWadPath, std::filesystem::path vAdditionalSearchDir, CIOGoldsrcWad& voWad);
    std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths, std::filesystem::path vAdditionalSearchDir = "");
}
