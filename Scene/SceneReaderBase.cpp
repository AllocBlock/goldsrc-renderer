#include "SceneReaderBase.h"

void CSceneReaderBase::__reportProgress(std::string vText)
{
    if (m_ProgressReportFunc) m_ProgressReportFunc(vText);
}