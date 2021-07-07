#pragma once
#include "SceneReaderBase.h"
#include "IOGoldSrcMdl.h"

class CSceneReaderMdl : public CSceneReaderBase
{
protected:
    virtual std::shared_ptr<SScene> _readV() override;
private:
    std::shared_ptr<S3DObject> __readBodyPart(const SMdlBodyPart& vBodyPart);
    void __readModel(const SMdlModel& vModel, std::shared_ptr<S3DObject> voObject);
};


