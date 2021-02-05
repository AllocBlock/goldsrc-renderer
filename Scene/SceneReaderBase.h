#pragma once
#include <string>
#include <functional>

class CSceneReaderBase
{
protected:
    void __reportProgress(std::string vText);

    std::function<void(std::string)> m_ProgressReportFunc = nullptr;
};

