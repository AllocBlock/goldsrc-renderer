#pragma once
#include "PipelineDepthTest.h"
class CPipelineBlendAlpha : public CPipelineDepthTest
{
protected:
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment) override;
};