#pragma once
#include "SceneInfo.h"

namespace SceneInterface
{
    void read(std::string vType, std::filesystem::path vFilePath, sptr<SSceneInfo> voSceneInfo);
}