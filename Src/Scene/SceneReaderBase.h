#pragma once
#include "Scene.h"

class CSceneReaderBase
{
public:
    ptr<SScene> read(std::filesystem::path vFilePath);
protected:
    virtual ptr<SScene> _readV() = 0;

    std::filesystem::path m_FilePath;
};