#pragma once
#include "PipelineGoldSrc.h"

class CPipelineDepthTest : public CPipelineGoldSrc
{
protected:
    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) override
    {
        vioDesc.setVertShaderPath("shaders/shaderVert.spv");
        vioDesc.setFragShaderPath("shaders/shaderFrag.spv");

        vioDesc.setEnableDepthTest(true);
        vioDesc.setEnableDepthWrite(true);
    }
};
