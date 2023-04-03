#pragma once
#include "Vulkan.h"

class CCommandBuffer
{
public:
    _DEFINE_PTR(CCommandBuffer);

    CCommandBuffer(VkCommandBuffer vBuffer, bool vIsSingleTimeBuffer);

    bool isValid();
    bool isBegun() { return m_IsBegun; }
    bool isPassBegun() { return m_IsPassBegun; }
    VkCommandBuffer get();
    void markAsOutdate() { m_Buffer = VK_NULL_HANDLE; }

    void begin();
    void end();

    void beginRenderPass(const VkRenderPassBeginInfo& vBeginInfo, bool vIsSecondary)
    {
        __assertValid();
        __assertBegun();
        if (m_IsPassBegun)
            throw std::runtime_error("Already begun a pass");
        m_IsPassBegun = true;
        VkSubpassContents Content = vIsSecondary ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE;
        vkCmdBeginRenderPass(m_Buffer, &vBeginInfo, Content);
    }

    void endRenderPass()
    {
        __assertValid();
        __assertBegun();
        if (!m_IsPassBegun)
            throw std::runtime_error("Should call beginRenderPass before endRenderPass");

        m_IsPassBegun = false;
        vkCmdEndRenderPass(m_Buffer);
    }

    // only single region for now
    void copyBuffer(VkBuffer vSrc, VkBuffer vDest, const VkBufferCopy& vCopyRegion)
    {
        __assertValid();
        __assertBegun();
        vkCmdCopyBuffer(m_Buffer, vSrc, vDest, 1, &vCopyRegion);
    }

    void copyBufferToImage(VkBuffer vSrcBuffer, VkImage vDestImage, VkImageLayout vTargetImageLayout, const VkBufferImageCopy& vCopyRegion)
    {
        __assertValid();
        __assertBegun();
        vkCmdCopyBufferToImage(m_Buffer, vSrcBuffer, vDestImage, vTargetImageLayout, 1, &vCopyRegion);
    }

    void copyImageToBuffer(VkImage vSrcImage, VkImageLayout vSrcImageLayout, VkBuffer vDestBuffer, const VkBufferImageCopy& vCopyRegion)
    {
        __assertValid();
        __assertBegun();
        vkCmdCopyImageToBuffer(m_Buffer, vSrcImage, vSrcImageLayout, vDestBuffer, 1, &vCopyRegion);
    }

    void addImageMemoryBarrier(VkPipelineStageFlags vSrcStage, VkPipelineStageFlags vDestStage, const VkImageMemoryBarrier& vBarrier)
    {
        __assertValid();
        __assertBegun();
        vkCmdPipelineBarrier(
            m_Buffer,
            vSrcStage, vDestStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &vBarrier
        );
    }

    void blitImage(VkImage vSrcImage, VkImageLayout vSrcLayout, VkImage vDestImage, VkImageLayout vDestLayout,
        const VkImageBlit& vBlitInfo, VkFilter vImageFilter)
    {
        __assertValid();
        __assertBegun();
        vkCmdBlitImage(m_Buffer,
            vSrcImage, vSrcLayout,
            vDestImage, vDestLayout,
            1, &vBlitInfo,
            vImageFilter);
    }

    void bindPipeline(VkPipeline vPipeline, VkPipelineLayout vPipelineLayout, VkDescriptorSet vDescriptorSet)
    {
        __assertValid();
        __assertBegun();
        if (m_BoundPipeline != vPipeline || m_BoundPipelineLayout != vPipelineLayout)
        {
            VkDeviceSize Offsets[] = { 0 };
            vkCmdBindPipeline(m_Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vPipeline);
            vkCmdBindDescriptorSets(m_Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vPipelineLayout, 0, 1, &vDescriptorSet, 0, nullptr);

            m_BoundPipeline = vPipeline;
            m_BoundPipelineLayout = vPipelineLayout;
        }
    }

    void bindVertexBuffer(VkBuffer vVertexBuffer)
    {
        __assertValid();
        __assertBegun();

        _ASSERTE(vVertexBuffer);
        if (m_BoundVertBuffer != vVertexBuffer)
        {
            VkDeviceSize Offsets[] = { 0 };
            vkCmdBindVertexBuffers(m_Buffer, 0, 1, &vVertexBuffer, Offsets);

            m_BoundVertBuffer = vVertexBuffer;
        }
    }

    void draw(uint32_t vStart, uint32_t vCount)
    {
        __assertValid();
        __assertBegun();
        __assertBoundVertexBuffer();
        __assertBoundPipeline();

        vkCmdDraw(m_Buffer, vCount, 1, vStart, 0);
    }


    template <typename T>
    void pushConstant(VkShaderStageFlags vState, const T& vPushConstant)
    {
        __assertValid();
        __assertBegun();
        __assertBoundPipeline();

        vkCmdPushConstants(m_Buffer, m_BoundPipelineLayout, vState, 0, sizeof(vPushConstant), &vPushConstant);
    }

private:
    // no copy
    CCommandBuffer(const CCommandBuffer&) = delete;
    CCommandBuffer& operator=(const CCommandBuffer&) = delete;

    void __assertValid();
    void __assertBegun()
    {
        if (!isBegun())
            throw std::runtime_error("Command buffer has not begun for recording command.");
    }
    void __assertPassBegun()
    {
        if (!isPassBegun())
            throw std::runtime_error("Command buffer has not begun a pass for recording command.");
    }
    void __assertBoundVertexBuffer()
    {
        if (m_BoundVertBuffer == VK_NULL_HANDLE)
            throw std::runtime_error("Bind vertex buffer before any draw.");
    }
    void __assertBoundPipeline()
    {
        if (m_BoundPipeline == VK_NULL_HANDLE)
            throw std::runtime_error("Bind pipeline before any draw.");
    }

    bool m_IsBegun = false;
    bool m_IsPassBegun = false;
    VkCommandBuffer m_Buffer = VK_NULL_HANDLE;
    bool m_IsSingleTimeBuffer;

    VkBuffer m_BoundVertBuffer = VK_NULL_HANDLE;
    VkPipeline m_BoundPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_BoundPipelineLayout = VK_NULL_HANDLE;
};