#include "SceneReaderBase.h"

std::shared_ptr<SScene> CSceneReaderBase::read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    m_FilePath = vFilePath;
    m_ProgressReportFunc = vProgressReportFunc;
    return _readV();
}

void CSceneReaderBase::_reportProgress(std::string vText)
{
    if (m_ProgressReportFunc) m_ProgressReportFunc(vText);
}