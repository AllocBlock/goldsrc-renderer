#pragma once
#include "SceneReaderBase.h"
#include "IOGoldSrcMdl.h"

class CSceneReaderMdl : public CSceneReaderBase
{
protected:
    virtual ptr<SSceneInfoGoldSrc> _readV() override;
private:
    CActor<CMeshDataGoldSrc>::Ptr __readBodyPart(const SMdlBodyPart& vBodyPart);
    void __appendModelData(const SMdlModel& vModel, CMeshDataGoldSrc& vioMeshData);

    ptr<CIOGoldSrcMdl> m_pIOMdl = nullptr;
};


