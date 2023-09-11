#pragma once
#include "PassFullScreen.h"
#include "PipelineBlit.h"

// blit (copy) image from input to swapchain
class CRenderPassPresent : public CRenderPassFullScreen
{
public:
    inline static const std::string Name = "Present";

    void setSwapchainPort(CSourcePort::Ptr vSwapchainPort);

protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    void _destroyV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override;
    virtual std::vector<VkClearValue> _getClearValuesV() override;

private:
    CPipelineBlit m_BlitPipeline;

    CSourcePort::Ptr m_pSwapchainPort = nullptr;
};
