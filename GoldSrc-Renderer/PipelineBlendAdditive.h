#pragma once
#include "PipelineDepthTest.h"
class CPipelineBlendAdditive : public CPipelineDepthTest
{
protected:
    VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV()
    {
        VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
        DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencilInfo.depthTestEnable = VK_TRUE;
        DepthStencilInfo.depthWriteEnable = VK_FALSE;
        DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        DepthStencilInfo.stencilTestEnable = VK_FALSE;

        return DepthStencilInfo;
    }

    virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment) override
    {
        // additive: 
        // result color = source color + old color
        // result alpha = source alpha + dst alpha
        voBlendAttachment = {};
        voBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        voBlendAttachment.blendEnable = VK_TRUE;
        voBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        voBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        voBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        voBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        voBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        voBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
};