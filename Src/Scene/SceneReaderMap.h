#pragma once
#include "SceneInfo.h"
#include "SceneReaderBase.h"

class CSceneReaderMap : public CSceneReaderBase
{
protected:
    virtual void _readV(sptr<SSceneInfo> voSceneInfo) override;
};
