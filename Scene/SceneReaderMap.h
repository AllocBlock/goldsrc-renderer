#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"

class CSceneReaderMap : public CSceneReaderBase
{
public:
    SScene read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc);
private:
    SScene m_Scene;
};

