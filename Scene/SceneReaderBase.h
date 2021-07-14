#pragma once
#include "Scene.h"

#include <string>
#include <functional>
#include <future>

class CSceneReaderBase
{
public:
    std::shared_ptr<SScene> read(std::filesystem::path vFilePath);
protected:
    virtual std::shared_ptr<SScene> _readV() = 0;

    std::filesystem::path m_FilePath;
};