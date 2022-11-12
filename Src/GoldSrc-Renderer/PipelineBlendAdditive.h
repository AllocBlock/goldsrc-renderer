#pragma once
#include "PipelineGoldSrc.h"

class CPipelineBlendAdditive : public CPipelineGoldSrc
{
protected:
    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) override
    {
        vioDesc.setVertShaderPath("shaders/goldSrcNormalShader.vert");
        vioDesc.setFragShaderPath("shaders/goldSrcNormalShader.frag");

        vioDesc.setEnableDepthTest(true);
        vioDesc.setEnableDepthWrite(false);

        // additive: 
        // result color = source color + old color
        // result alpha = source alpha + dst alpha
        vioDesc.setEnableBlend(true);
        vioDesc.setBlendMethod(EBlendFunction::ADDITIVE);
    }
};
