#pragma once
#include "IOImage.h"
#include "IOGoldsrcWad.h"
#include "IOGoldsrcBsp.h"

#include <memory>
#include <functional>

namespace Scene
{
    sptr<CIOImage> generateGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize, uint8_t vBaseColor1[3], uint8_t vBaseColor2[3]);
    sptr<CIOImage> generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize);
    sptr<CIOImage> generatePureColorTexture(uint8_t vBaseColor[3], size_t vSize);
    sptr<CIOImage> generateDiagonalGradientGrid(size_t vWidth, size_t vHeight, uint8_t vR1, uint8_t vG1, uint8_t vB1, uint8_t vR2, uint8_t vG2, uint8_t vB2);
    sptr<CIOImage> getIOImageFromWad(const CIOGoldsrcWad& vWad, size_t vIndex);
    sptr<CIOImage> getIOImageFromBspTexture(const SBspTexture& vBspTexture);

    template <typename T>
    struct SRequestResult
    {
        bool Found = false;
        T Data;
    };

    using SRequestResultFilePath = SRequestResult<std::filesystem::path>;

    void reportProgress(std::string vText);
    void setGlobalReportProgressFunc(std::function<void(std::string)> vFunc);
    void setGlobalRequestFilePathFunc(std::function<SRequestResultFilePath(const std::filesystem::path&, const std::filesystem::path&, const std::string&, const std::string&)> vFunc);

    SRequestResultFilePath requestFilePath(const std::filesystem::path& vOriginPath, const std::filesystem::path& vAdditionalSearchDir, const std::string& vMessage, const std::string& vFilter = "");
}