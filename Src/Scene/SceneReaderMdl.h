#pragma once
#include "SceneReaderBase.h"
#include "IOGoldSrcMdl.h"

class CSceneReaderMdl : public CSceneReaderBase
{
protected:
    virtual ptr<SScene> _readV() override;
private:
    ptr<CMeshDataGoldSrc> __readBodyPart(const SMdlBodyPart& vBodyPart);
    void __readModel(const SMdlModel& vModel, ptr<CMeshDataGoldSrc> voObject);

    ptr<CIOGoldSrcMdl> m_pIOMdl = nullptr;
};


