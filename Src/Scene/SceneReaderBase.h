#pragma once
#include "SceneInfo.h"

class CSceneReaderBase
{
public:
    void read(std::filesystem::path vFilePath, ptr<SSceneInfo> voSceneInfo);
protected:
    virtual void _readV(ptr<SSceneInfo> voSceneInfo) = 0;

    std::filesystem::path m_FilePath;
};