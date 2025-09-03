#pragma once
#include "SceneReaderBase.h"
#include "IOGoldSrcMdl.h"

class CSceneReaderMdl : public CSceneReaderBase
{
protected:
    virtual void _readV(sptr<SSceneInfo> voSceneInfo) override;
private:
    sptr<CActor> __readBodyPart(const SMdlBodyPart& vBodyPart);
    void __appendModelData(const SMdlModel& vModel, CMeshData& vioMeshData);

    sptr<CIOGoldSrcMdl> m_pIOMdl = nullptr;
};


