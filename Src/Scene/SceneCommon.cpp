#include "Common.h"
#include "SceneCommon.h"
#include "Environment.h"

std::function<void(std::string)> g_pReportProgressFunc = nullptr;
std::function<Scene::SRequestResultFilePath(std::string,std::string)> g_pRequestFilePathFunc = nullptr;

ptr<CIOImage> Scene::generateGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize, uint8_t vBaseColor1[3], uint8_t vBaseColor2[3])
{
    size_t DataSize = vNumRow * vNumCol * vCellSize * vCellSize * 4;
    uint8_t* pIndices = new uint8_t[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        size_t GridRowIndex = (i / (vNumCol * vCellSize)) / vCellSize;
        size_t GridColIndex = (i % (vNumCol * vCellSize)) / vCellSize;

        uint8_t* pColor;
        if ((GridRowIndex + GridColIndex) % 2 == 0)
            pColor = vBaseColor1;
        else
            pColor = vBaseColor2;
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
    ptr<CIOImage> pGrid = make<CIOImage>();
    pGrid->setSize(vNumCol * vCellSize, vNumRow * vCellSize);
    pGrid->setData(pIndices);

    return pGrid;
}

ptr<CIOImage> Scene::generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize)
{
    uint8_t BaseColor1[3] = { 0, 0, 0 };
    uint8_t BaseColor2[3] = { 255, 0, 255 };
    return generateGrid(vNumRow, vNumCol, vCellSize, BaseColor1, BaseColor2);
}

ptr<CIOImage> Scene::generatePureColorTexture(uint8_t vBaseColor[3], size_t vSize)
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

    ptr<CIOImage> pPure = make<CIOImage>();
    pPure->setSize(vSize, vSize);
    pPure->setData(pIndices);

    return pPure;
}

ptr<CIOImage> Scene::generateDiagonalGradientGrid(size_t vWidth, size_t vHeight, uint8_t vR1, uint8_t vG1, uint8_t vB1, uint8_t vR2, uint8_t vG2, uint8_t vB2)
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
            size_t StartIndex = static_cast<size_t>(i * RowSize + k * 4);
            pIndices[StartIndex] = Common::lerp<uint8_t>(BaseColor1[0], BaseColor2[0], Factor);
            pIndices[StartIndex + 1] = Common::lerp<uint8_t>(BaseColor1[1], BaseColor2[1], Factor);
            pIndices[StartIndex + 2] = Common::lerp<uint8_t>(BaseColor1[2], BaseColor2[2], Factor);
            pIndices[StartIndex + 3] = static_cast<uint8_t>(255);
        }
    }

    ptr<CIOImage> pGradient = make<CIOImage>();
    pGradient->setSize(vWidth, vHeight);
    pGradient->setData(pIndices);

    return pGradient;
}

ptr<CIOImage> Scene::getIOImageFromWad(const CIOGoldsrcWad& vWad, size_t vIndex)
{
    uint32_t Width = 0, Height = 0;
    vWad.getTextureSize(vIndex, Width, Height);

    ptr<CIOImage> pTexImage = make<CIOImage>();
    pTexImage->setSize(static_cast<int>(Width), static_cast<int>(Height));
    void* pIndices = new uint8_t[static_cast<size_t>(4) * Width * Height];
    vWad.getRawRGBAPixels(vIndex, pIndices);
    pTexImage->setName(vWad.getTextureName(vIndex));
    pTexImage->setData(pIndices);
    pTexImage->setName(vWad.getTextureName(vIndex));
    delete[] pIndices;
    return pTexImage;
}

ptr<CIOImage> Scene::getIOImageFromBspTexture(const SBspTexture& vBspTexture)
{
    uint8_t* pIndices = new uint8_t[static_cast<size_t>(4) * vBspTexture.Width * vBspTexture.Height];
    vBspTexture.getRawRGBAPixels(pIndices);
    ptr<CIOImage> pTexImage = make<CIOImage>();
    pTexImage->setSize(vBspTexture.Width, vBspTexture.Height);
    pTexImage->setName(vBspTexture.Name);
    pTexImage->setData(pIndices);
    delete[] pIndices;
    return pTexImage;
}

void Scene::reportProgress(std::string vText)
{
    if (g_pReportProgressFunc) g_pReportProgressFunc(vText);
}

void Scene::setGlobalReportProgressFunc(std::function<void(std::string)> vFunc)
{
    g_pReportProgressFunc = vFunc;
}

Scene::SRequestResultFilePath Scene::requestFilePath(std::string vMessage, std::string vFilter)
{
    if (g_pRequestFilePathFunc) return g_pRequestFilePathFunc(vMessage, vFilter);
    else return { ERequestResultState::IGNORE_, "" };
}

void Scene::setGlobalRequestFilePathFunc(std::function<SRequestResultFilePath(std::string, std::string)> vFunc)
{
    g_pRequestFilePathFunc = vFunc;
}

bool Scene::requestFilePathUntilCancel(std::filesystem::path vFilePath, std::filesystem::path vAdditionalSearchDir, std::string vFilter, std::filesystem::path& voFilePath)
{
    while (true)
    {
        if (Environment::findFile(vFilePath, vAdditionalSearchDir, true, vFilePath))
        {
            voFilePath = vFilePath;
            return true;
        }
        else
        {
            Scene::SRequestResultFilePath FilePathResult;
            FilePathResult = Scene::requestFilePath(u8"需要文件：" + vFilePath.u8string(), vFilter);
            if (FilePathResult.State == Scene::ERequestResultState::CONTINUE)
            {
                vFilePath = FilePathResult.Data;
            }
            else
            {
                return false;
            }
        }
    }
}