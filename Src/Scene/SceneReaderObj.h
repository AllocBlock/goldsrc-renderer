#pragma once
#include "SceneInfo.h"
#include "SceneReaderBase.h"

class CSceneReaderObj : public CSceneReaderBase
{
protected:
    virtual void _readV(sptr<SSceneInfo> voSceneInfo) override;
};

