#pragma once
#include "Scene.h"

namespace SceneReader
{
    ptr<SScene> read(std::string vType, std::filesystem::path vFilePath);
}