#pragma once
#include "PipelineGoldSrc.h"

class CPipelineBlendAlpha : public CPipelineGoldSrc
{
protected:
    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) override
    {
        vioDesc.setVertShaderPath("shaders/shaderVert.spv");
        vioDesc.setFragShaderPath("shaders/shaderFrag.spv");

        vioDesc.setEnableDepthTest(true);
        vioDesc.setEnableDepthWrite(false);
        
        vioDesc.setEnableBlend(true);
        vioDesc.setBlendMethod(EBlendFunction::NORMAL);
    }
};
