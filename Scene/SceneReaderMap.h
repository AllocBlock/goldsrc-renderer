#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"

class CSceneReaderMap : public CSceneReaderBase
{
public:
    std::shared_ptr<SScene> read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc);
private:
    std::shared_ptr<SScene> m_pScene = nullptr;
};

