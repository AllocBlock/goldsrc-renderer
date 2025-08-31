#pragma once
#include "RenderPass.h"
#include "PipelineBlit.h"
#include "FullScreenPointData.h"
#include "Swapchain.h"
#include "RenderInfoDescriptor.h"

// blit (copy) image from input to swapchain
class CRenderPassPresent : public engine::IRenderPass
{
public:
    inline static const std::string Name = "Present";

    CRenderPassPresent(wptr<vk::CSwapchain> vpSwapchain);
    void updateSwapchainImageIndex(uint32_t vImageIndex);

protected:
    virtual CPortSet::Ptr _createPortSetV() final;
    virtual void _initV() override;
    virtual void _destroyV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;

private:
    void __createCommandPoolAndBuffers();
    void __destroyCommandPoolAndBuffers();
    void __createVertexBuffer();
    void __generateScene();
    void __transitLayoutToPresent(CCommandBuffer::Ptr vCommandBuffer);

    const std::string m_DefaultCommandName = "Default";

    wptr<vk::CSwapchain> m_pSwapchain;
    uint32_t m_CurrentSwapchainImageIndex = 0;

    CCommand m_Command = CCommand();
    ptr<vk::CVertexBufferTyped<SFullScreenPointData>> m_pVertexBuffer = nullptr;
    std::vector<SFullScreenPointData> m_PointDataSet;
    std::vector<CRenderInfoDescriptor> m_RenderInfoDescriptors;
    CPipelineBlit m_BlitPipeline;

};