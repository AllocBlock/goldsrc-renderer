#pragma once
#include "SceneInfoGoldSrc.h"

class CSceneReaderBase
{
public:
    ptr<SSceneInfoGoldSrc> read(std::filesystem::path vFilePath);
protected:
    virtual ptr<SSceneInfoGoldSrc> _readV() = 0;

    std::filesystem::path m_FilePath;
};