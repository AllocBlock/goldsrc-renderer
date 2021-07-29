#pragma once
#include "PipelineDepthTest.h"
class CPipelineBlendAdditive : public CPipelineDepthTest
{
protected:
    virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment) override
    {
        // result color = source color * source alpha + old color * (1 - source alpha)
        voBlendAttachment = {};
        voBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        voBlendAttachment.blendEnable = VK_TRUE;
        voBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
        voBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        voBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        voBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        voBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        voBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
};