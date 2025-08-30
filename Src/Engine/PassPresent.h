#pragma once
#include "PassFullScreen.h"
#include "PipelineBlit.h"
#include "Swapchain.h"

// blit (copy) image from input to swapchain
class CRenderPassPresent : public vk::IRenderPass
{
public:
    inline static const std::string Name = "Present";

    CRenderPassPresent(wptr<vk::CSwapchain> vpSwapchain);
    void updateSwapchainImageIndex(uint32_t vImageIndex);

protected:
    static const std::vector<VkClearValue>& DefaultClearValueColor;
    void _bindVertexBuffer(CCommandBuffer::Ptr vCommandBuffer);
    void _drawFullScreen(CCommandBuffer::Ptr vCommandBuffer);

    // 3
protected:
    virtual CPortSet::Ptr _createPortSetV() final;
    virtual void _initV() override;
    void _destroyV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;

private:
    // hide original begin/end, use _beginWithFramebuffer/_endWithFramebuffer instead
    using vk::IRenderPass::_begin;
    using vk::IRenderPass::_end;

    void __createCommandPoolAndBuffers();
    void __destroyCommandPoolAndBuffers();
    void __createFramebuffers(VkExtent2D vExtent);
    void __createVertexBuffer();
    void __generateScene();

    const std::string m_DefaultCommandName = "Default";

    wptr<vk::CSwapchain> m_pSwapchain;
    uint32_t m_CurrentSwapchainImageIndex = 0;

    CCommand m_Command = CCommand();
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    std::vector<VkClearValue> m_ClearValueSet;
    ptr<vk::CVertexBufferTyped<SFullScreenPointData>> m_pVertexBuffer = nullptr;
    std::vector<SFullScreenPointData> m_PointDataSet;
    CPipelineBlit m_BlitPipeline;
};