#pragma once
#include "SceneInfo.h"

class CSceneReaderBase
{
public:
    void read(std::filesystem::path vFilePath, sptr<SSceneInfo> voSceneInfo);
protected:
    virtual void _readV(sptr<SSceneInfo> voSceneInfo) = 0;

    std::filesystem::path m_FilePath;
};