#include "SceneReaderBase.h"

ptr<SScene> CSceneReaderBase::read(std::filesystem::path vFilePath)
{
    m_FilePath = vFilePath;
    return _readV();
}