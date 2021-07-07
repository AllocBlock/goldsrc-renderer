#pragma once
#include "Scene.h"

#include <string>
#include <functional>

class CSceneReaderBase
{
public:
    std::shared_ptr<SScene> read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc);
protected:
    virtual std::shared_ptr<SScene> _readV() = 0;
    void _reportProgress(std::string vText);

    std::filesystem::path m_FilePath;
    std::function<void(std::string)> m_ProgressReportFunc = nullptr;
};

