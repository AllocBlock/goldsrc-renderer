#include "SceneCommon.h"

std::shared_ptr<CIOImage> generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize)
{
    uint8_t BaseColor1[3] = { 0, 0, 0 };
    uint8_t BaseColor2[3] = { 255, 0, 255 };
    size_t DataSize = vNumRow * vNumCol * vCellSize * vCellSize * 4;
    uint8_t* pIndices = new uint8_t[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        size_t GridRowIndex = (i / (vNumCol * vCellSize)) / vCellSize;
        size_t GridColIndex = (i % (vNumCol * vCellSize)) / vCellSize;

        uint8_t* pColor;
        if ((GridRowIndex + GridColIndex) % 2 == 0)
            pColor = BaseColor1;
        else
            pColor = BaseColor2;
        pIndices[i * 4] = pColor[0];
        pIndices[i * 4 + 1] = pColor[1];
        pIndices[i * 4 + 2] = pColor[2];
        pIndices[i * 4 + 3] = static_cast<uint8_t>(255);
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
    pGrid->setData(pIndices);

    return pGrid;
}

std::shared_ptr<CIOImage> generatePureColorTexture(uint8_t vBaseColor[3], size_t vSize)
{
    size_t DataSize = vSize * vSize * 4;
    uint8_t* pIndices = new uint8_t[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        pIndices[i * 4] = vBaseColor[0];
        pIndices[i * 4 + 1] = vBaseColor[1];
        pIndices[i * 4 + 2] = vBaseColor[2];
        pIndices[i * 4 + 3] = static_cast<uint8_t>(255);
    }

    std::shared_ptr<CIOImage> pPure = std::make_shared<CIOImage>();
    pPure->setImageSize(vSize, vSize);
    pPure->setData(pIndices);

    return pPure;
}

std::shared_ptr<CIOImage> generateDiagonalGradientGrid(size_t vWidth, size_t vHeight, uint8_t vR1, uint8_t vG1, uint8_t vB1, uint8_t vR2, uint8_t vG2, uint8_t vB2)
{
    uint8_t BaseColor1[3] = { vR1, vG1, vB1 };
    uint8_t BaseColor2[3] = { vR2, vG2, vB2 };
    size_t DataSize = vWidth * vHeight * 4;
    uint8_t* pIndices = new uint8_t[DataSize];

    size_t RowSize = vWidth * 4;
    for (size_t i = 0; i < vHeight; ++i)
    {
        float FactorX = 1 - static_cast<float>(i) / (vHeight - 1);
        for (size_t k = 0; k < vWidth; ++k)
        {
            float FactorY = 1 - static_cast<float>(k) / (vWidth - 1);
            float Factor = (FactorX + FactorY) / 2;
            pIndices[i * RowSize + k * 4] = BaseColor1[0] * Factor + BaseColor2[0] * (1 - Factor);
            pIndices[i * RowSize + k * 4 + 1] = BaseColor1[1] * Factor + BaseColor2[1] * (1 - Factor);
            pIndices[i * RowSize + k * 4 + 2] = BaseColor1[2] * Factor + BaseColor2[2] * (1 - Factor);
            pIndices[i * RowSize + k * 4 + 3] = static_cast<uint8_t>(255);
        }
    }

    std::shared_ptr<CIOImage> pGradient = std::make_shared<CIOImage>();
    pGradient->setImageSize(vWidth, vHeight);
    pGradient->setData(pIndices);

    return pGradient;
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
    void* pIndices = new uint8_t[static_cast<size_t>(4) * Width * Height];
    vWad.getRawRGBAPixels(vIndex, pIndices);
    pTexImage->setData(pIndices);
    delete[] pIndices;
    return pTexImage;
}

std::shared_ptr<CIOImage> getIOImageFromBspTexture(const SBspTexture& vBspTexture)
{
    uint8_t* pIndices = new uint8_t[static_cast<size_t>(4) * vBspTexture.Width * vBspTexture.Height];
    vBspTexture.getRawRGBAPixels(pIndices);
    std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
    pTexImage->setImageSize(vBspTexture.Width, vBspTexture.Height);
    pTexImage->setData(pIndices);
    delete[] pIndices;
    return pTexImage;
}