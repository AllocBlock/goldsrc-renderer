#include "SceneReaderBase.h"

ptr<SSceneInfoGoldSrc> CSceneReaderBase::read(std::filesystem::path vFilePath)
{
    m_FilePath = vFilePath;
    return _readV();
}