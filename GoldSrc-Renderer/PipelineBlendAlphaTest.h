#pragma once
#include "PipelineDepthTest.h"
class CPipelineBlendAlphaTest : public CPipelineDepthTest
{
protected:
    // opacity (renderamt) is not allow for alpha test (solid render mode)
    virtual std::filesystem::path _getVertShaderPathV() override { return "shader/alphaTestShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shader/alphaTestShaderFrag.spv"; }
};