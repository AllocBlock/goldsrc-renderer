#pragma once
#include "SceneReaderBase.h"
#include "IOGoldSrcMdl.h"

class CSceneReaderMdl : public CSceneReaderBase
{
protected:
    virtual void _readV(ptr<SSceneInfo> voSceneInfo) override;
private:
    CActor::Ptr __readBodyPart(const SMdlBodyPart& vBodyPart);
    void __appendModelData(const SMdlModel& vModel, CMeshData& vioMeshData);

    ptr<CIOGoldSrcMdl> m_pIOMdl = nullptr;
};


