#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"

class CSceneReaderObj : public CSceneReaderBase
{
protected:
    virtual ptr<SScene> _readV() override;
private:
    ptr<SScene> m_pScene = nullptr;
};
