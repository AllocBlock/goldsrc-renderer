#pragma once
#include "SceneReaderBase.h"
#include "IOGoldSrcMdl.h"

class CSceneReaderMdl : public CSceneReaderBase
{
protected:
    virtual ptr<SScene> _readV() override;
private:
    ptr<C3DObjectGoldSrc> __readBodyPart(const SMdlBodyPart& vBodyPart);
    void __readModel(const SMdlModel& vModel, ptr<C3DObjectGoldSrc> voObject);

    ptr<CIOGoldSrcMdl> m_pIOMdl = nullptr;
};


