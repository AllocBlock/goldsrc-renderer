#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"

class CSceneReaderMap : public CSceneReaderBase
{
protected:
    virtual std::shared_ptr<SScene> _readV() override;
private:
    std::shared_ptr<SScene> m_pScene = nullptr;
};

