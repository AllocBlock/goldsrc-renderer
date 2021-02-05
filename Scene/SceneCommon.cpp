#include "SceneCommon.h"

std::shared_ptr<CIOImage> generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize)
{
    uint8_t BaseColor1[3] = { 0, 0, 0 };
    uint8_t BaseColor2[3] = { 255, 0, 255 };
    size_t DataSize = vNumRow * vNumCol * vCellSize * vCellSize * 4;
    uint8_t* pData = new uint8_t[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        size_t GridRowIndex = (i / (vNumCol * vCellSize)) / vCellSize;
        size_t GridColIndex = (i % (vNumCol * vCellSize)) / vCellSize;

        uint8_t* pColor;
        if ((GridRowIndex + GridColIndex) % 2 == 0)
            pColor = BaseColor1;
        else
            pColor = BaseColor2;
        pData[i * 4] = pColor[0];
        pData[i * 4 + 1] = pColor[1];
        pData[i * 4 + 2] = pColor[2];
        pData[i * 4 + 3] = static_cast<uint8_t>(255);
    }

    // cout 
    /*for (size_t i = 0; i < vNumRow * vCellSize; i++)
    {
        for (size_t k = 0; k < vNumCol * vCellSize; k++)
        {
            std::cout << pData[(i * vNumCol * vCellSize + k) * 4] % 2 << " ";
        }
        std::cout << std::endl;
    }*/
    std::shared_ptr<CIOImage> pGrid = std::make_shared<CIOImage>();
    pGrid->setImageSize(vNumCol * vCellSize, vNumRow * vCellSize);
    pGrid->setData(pData);

    return pGrid;
}

std::shared_ptr<CIOImage> generatePureColorTexture(uint8_t vBaseColor[3], size_t vSize)
{
    size_t DataSize = vSize * vSize * 4;
    uint8_t* pData = new uint8_t[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        pData[i * 4] = vBaseColor[0];
        pData[i * 4 + 1] = vBaseColor[1];
        pData[i * 4 + 2] = vBaseColor[2];
        pData[i * 4 + 3] = static_cast<uint8_t>(255);
    }

    std::shared_ptr<CIOImage> pGrid = std::make_shared<CIOImage>();
    pGrid->setImageSize(vSize, vSize);
    pGrid->setData(pData);

    return pGrid;
}

bool findFile(std::filesystem::path vFilePath, std::filesystem::path vSearchDir, std::filesystem::path& voFilePath)
{
    std::filesystem::path CurPath(vFilePath);

    std::filesystem::path FullPath = std::filesystem::absolute(vFilePath);
    std::filesystem::path CurDir = FullPath.parent_path();
    while (!CurDir.empty() && CurDir != CurDir.parent_path())
    {
        std::filesystem::path SearchPath = std::filesystem::relative(FullPath, CurDir);
        std::filesystem::path CombinedSearchPath = std::filesystem::path(vSearchDir) / SearchPath;
        if (std::filesystem::exists(SearchPath))
        {
            voFilePath = SearchPath;
            return true;
        }
        else if (std::filesystem::exists(CombinedSearchPath))
        {
            voFilePath = CombinedSearchPath;
            return true;
        }
        CurDir = CurDir.parent_path();
    }

    return false;
}

std::shared_ptr<CIOImage> getIOImageFromWad(const CIOGoldsrcWad& vWad, size_t vIndex)
{
    uint32_t Width = 0, Height = 0;
    vWad.getTextureSize(vIndex, Width, Height);

    std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
    pTexImage->setImageSize(static_cast<int>(Width), static_cast<int>(Height));
    void* pData = new uint8_t[static_cast<size_t>(4) * Width * Height];
    vWad.getRawRGBAPixels(vIndex, pData);
    pTexImage->setData(pData);
    delete[] pData;
    return pTexImage;
}

std::shared_ptr<CIOImage> getIOImageFromBspTexture(const SBspTexture& vBspTexture)
{
    uint8_t* pData = new uint8_t[static_cast<size_t>(4) * vBspTexture.Width * vBspTexture.Height];
    vBspTexture.getRawRGBAPixels(pData);
    std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
    pTexImage->setImageSize(vBspTexture.Width, vBspTexture.Height);
    pTexImage->setData(pData);
    delete[] pData;
    return pTexImage;
}