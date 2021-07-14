#include "SceneReaderBase.h"

std::shared_ptr<SScene> CSceneReaderBase::read(std::filesystem::path vFilePath)
{
    m_FilePath = vFilePath;
    return _readV();
}