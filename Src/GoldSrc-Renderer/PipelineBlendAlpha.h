#pragma once
#include "PipelineGoldSrc.h"

class CPipelineBlendAlpha : public CPipelineGoldSrc
{
protected:
    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) override
    {
        vioDesc.setVertShaderPath("shaders/goldSrcNormalShader.vert");
        vioDesc.setFragShaderPath("shaders/goldSrcNormalShader.frag");

        vioDesc.setEnableDepthTest(true);
        vioDesc.setEnableDepthWrite(false);
        
        vioDesc.setEnableBlend(true);
        vioDesc.setBlendMethod(EBlendFunction::NORMAL);
    }
};
