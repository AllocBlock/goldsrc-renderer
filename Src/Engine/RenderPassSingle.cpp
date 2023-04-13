#include "RenderPassSingleFrameBuffer.h"

std::vector<VkClearValue> __createDefaultClearValueColor()
{
    VkClearValue Value;
    Value.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    return { Value };
}

std::vector<VkClearValue> __createDefaultClearValueColorDepth()
{
    std::vector<VkClearValue> ValueSet(2);
    ValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ValueSet[1].depthStencil = { 1.0f, 0 };

    return ValueSet;
}

const std::vector<VkClearValue>& CRenderPassSingleFrameBuffer::DefaultClearValueColor = __createDefaultClearValueColor();
const std::vector<VkClearValue>& CRenderPassSingleFrameBuffer::DefaultClearValueColorDepth = __createDefaultClearValueColorDepth();