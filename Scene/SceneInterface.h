#pragma once
#include "Scene.h"

namespace SceneReader
{
    std::shared_ptr<SScene> read(std::string vType, std::filesystem::path vFilePath);
}