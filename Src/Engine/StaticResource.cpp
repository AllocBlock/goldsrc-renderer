#include "StaticResource.h"

std::filesystem::path __getResourceAbsPath(std::filesystem::path vRelativePath)
{
    return std::filesystem::absolute(StaticResource::ResourceDir / vRelativePath);
}

CIOImage::Ptr StaticResource::loadImage(std::filesystem::path vRelativePath)
{
    std::filesystem::path FilePath = __getResourceAbsPath(vRelativePath);
    CIOImage::Ptr pImage = make<CIOImage>();
    pImage->read(FilePath);
    return pImage;
}