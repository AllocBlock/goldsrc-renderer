#pragma once
#include "SceneInfoGoldSrc.h"

namespace SceneInterface
{
    ptr<SSceneInfoGoldSrc> read(std::string vType, std::filesystem::path vFilePath);
}