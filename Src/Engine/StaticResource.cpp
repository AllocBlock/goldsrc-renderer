#include "StaticResource.h"

std::filesystem::path StaticResource::getAbsPath(std::filesystem::path vRelativePath)
{
    return std::filesystem::absolute(StaticResource::ResourceDir / vRelativePath);
}

sptr<CIOImage> StaticResource::loadImage(std::filesystem::path vRelativePath)
{
    std::filesystem::path FilePath = getAbsPath(vRelativePath);
    sptr<CIOImage> pImage = make<CIOImage>();
    pImage->read(FilePath);
    return pImage;
}