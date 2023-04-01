#pragma once
#include "PipelineGoldSrc.h"

// opacity (renderamt) is not allow for alpha test (solid render mode)
class CPipelineBlendAlphaTest : public CPipelineGoldSrc
{
protected:
    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) override
    {
        vioDesc.setVertShaderPath("alphaTestShader.vert");
        vioDesc.setFragShaderPath("alphaTestShader.frag");

        vioDesc.setEnableDepthTest(true);
        vioDesc.setEnableDepthWrite(true);
    }
};
