#pragma once
#include "IOImage.h"

namespace StaticResource
{
    const std::filesystem::path ResourceDir = "../../Resources/";
    CIOImage::Ptr loadImage(std::filesystem::path vRelativePath);
};