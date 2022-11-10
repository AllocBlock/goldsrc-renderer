#pragma once
#include "PipelineGoldSrc.h"

class CPipelineDepthTest : public CPipelineGoldSrc
{
protected:
    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) override
    {
        vioDesc.setVertShaderPath("shaders/shader.vert");
        vioDesc.setFragShaderPath("shaders/shader.frag");

        vioDesc.setEnableDepthTest(true);
        vioDesc.setEnableDepthWrite(true);
    }
};
