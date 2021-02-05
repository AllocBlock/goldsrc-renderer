#pragma once

#include <memory>
#include "IOImage.h"
#include "IOGoldsrcWad.h"
#include "IOGoldsrcBsp.h"

std::shared_ptr<CIOImage> generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize);
std::shared_ptr<CIOImage> generatePureColorTexture(uint8_t vBaseColor[3], size_t vSize);
bool findFile(std::filesystem::path vFilePath, std::filesystem::path vSearchDir, std::filesystem::path& voFilePath);
std::shared_ptr<CIOImage> getIOImageFromWad(const CIOGoldsrcWad& vWad, size_t vIndex);
std::shared_ptr<CIOImage> getIOImageFromBspTexture(const SBspTexture& vBspTexture);