#pragma once

#include <memory>
#include <vector>
#include "IOImage.h"
#include "IOGoldsrcWad.h"
#include "IOGoldsrcBsp.h"

namespace Common
{
    namespace Scene
    {
        ptr<CIOImage> generateGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize, uint8_t vBaseColor1[3], uint8_t vBaseColor2[3]);
        ptr<CIOImage> generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize);
        ptr<CIOImage> generatePureColorTexture(uint8_t vBaseColor[3], size_t vSize);
        bool findFile(std::filesystem::path vFilePath, std::filesystem::path vSearchDir, std::filesystem::path& voFilePath);
        bool findFile(std::filesystem::path vFilePath, const std::vector<std::filesystem::path>& vSearchDirs, std::filesystem::path& voFilePath);
        ptr<CIOImage> generateDiagonalGradientGrid(size_t vWidth, size_t vHeight, uint8_t vR1, uint8_t vG1, uint8_t vB1, uint8_t vR2, uint8_t vG2, uint8_t vB2);
        ptr<CIOImage> getIOImageFromWad(const CIOGoldsrcWad& vWad, size_t vIndex);
        ptr<CIOImage> getIOImageFromBspTexture(const SBspTexture& vBspTexture);

        enum class ERequestResultState
        {
            CONTINUE,
            RETRY,
            IGNORE_, // IGNORE已有默认宏定义，所以加了个下划线
            CANCEL
        };

        template <typename T>
        struct SRequestResult
        {
            ERequestResultState State = ERequestResultState::IGNORE_;
            T Data;
        };

        using SRequestResultFilePath = SRequestResult<std::filesystem::path>;

        void reportProgress(std::string vText);
        void setGlobalReportProgressFunc(std::function<void(std::string)> vFunc);
        SRequestResultFilePath requestFilePath(std::string vMessage, std::string vFilter = "");
        void setGlobalRequestFilePathFunc(std::function<SRequestResultFilePath(std::string, std::string)> vFunc);

        bool requestFilePathUntilCancel(std::filesystem::path vFilePath, std::filesystem::path vAdditionalSearchDir, std::string vFilter, std::filesystem::path& voFilePath);
    }
}