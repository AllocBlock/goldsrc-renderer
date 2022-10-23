#pragma once
#include "SceneInfoGoldSrc.h"
#include "SceneReaderBase.h"

class CSceneReaderObj : public CSceneReaderBase
{
protected:
    virtual ptr<SSceneInfoGoldSrc> _readV() override;
private:
    ptr<SSceneInfoGoldSrc> m_pSceneInfo = nullptr;
};

