#include "SceneReaderBase.h"

void CSceneReaderBase::read(std::filesystem::path vFilePath, ptr<SSceneInfo> voSceneInfo)
{
    _ASSERTE(voSceneInfo);
    m_FilePath = vFilePath;
    voSceneInfo->clear();
    _readV(voSceneInfo);
}