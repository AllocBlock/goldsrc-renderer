#pragma once
#include "Vulkan.h"
#include "Common.h"
#include "FrameBuffer.h"
#include "Buffer.h"
#include "RenderPass.h"
#include "PipelineOutlineEdge.h"

class CRenderPassOutlineEdge : public vk::IRenderPass
{
protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createFramebuffers(VkExtent2D vExtent);
    void __createVertexBuffer();

    CPipelineEdge m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<CPipelineEdge::SPointData> m_PointDataSet;
};
