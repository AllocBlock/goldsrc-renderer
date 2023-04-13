#pragma once
#include "SceneInfo.h"
#include "SceneReaderBase.h"

class CSceneReaderMap : public CSceneReaderBase
{
protected:
    virtual void _readV(ptr<SSceneInfo> voSceneInfo) override;
};
