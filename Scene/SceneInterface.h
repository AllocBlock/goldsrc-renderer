#pragma once
#include "Scene.h"
#include "SceneReaderObj.h"
#include "SceneReaderRmf.h"
#include "SceneReaderMap.h"
#include "SceneReaderBsp.h"

namespace SceneReader
{
    std::shared_ptr<SScene> read(std::string vType, std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);
}