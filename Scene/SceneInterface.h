#pragma once
#include "Scene.h"
#include "SceneReaderObj.h"
#include "SceneReaderMap.h"
#include "SceneReaderBsp.h"

namespace SceneReader
{
    SScene read(std::string vType, std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
}