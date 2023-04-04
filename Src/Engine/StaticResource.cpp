#include "StaticResource.h"

std::filesystem::path StaticResource::getAbsPath(std::filesystem::path vRelativePath)
{
    return std::filesystem::absolute(StaticResource::ResourceDir / vRelativePath);
}

CIOImage::Ptr StaticResource::loadImage(std::filesystem::path vRelativePath)
{
    std::filesystem::path FilePath = getAbsPath(vRelativePath);
    CIOImage::Ptr pImage = make<CIOImage>();
    pImage->read(FilePath);
    return pImage;
}