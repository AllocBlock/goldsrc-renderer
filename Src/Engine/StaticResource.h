#pragma once
#include "IOImage.h"

namespace StaticResource
{
    const std::filesystem::path ResourceDir = "../../Resources/";
    std::filesystem::path getAbsPath(std::filesystem::path vRelativePath);
    CIOImage::Ptr loadImage(std::filesystem::path vRelativePath);
};