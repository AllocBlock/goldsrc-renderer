#pragma once
#include "SceneInfo.h"
#include "SceneReaderBase.h"

class CSceneReaderObj : public CSceneReaderBase
{
protected:
    virtual void _readV(ptr<SSceneInfo> voSceneInfo) override;
};

