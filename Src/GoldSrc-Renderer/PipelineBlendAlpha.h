#pragma once
#include "PipelineGoldSrc.h"

class CPipelineBlendAlpha : public CPipelineGoldSrc
{
protected:
    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) override
    {
        vioDesc.setVertShaderPath("shaders/shader.vert");
        vioDesc.setFragShaderPath("shaders/shader.frag");

        vioDesc.setEnableDepthTest(true);
        vioDesc.setEnableDepthWrite(false);
        
        vioDesc.setEnableBlend(true);
        vioDesc.setBlendMethod(EBlendFunction::NORMAL);
    }
};
