#pragma once
#include "RenderPass.h"
#include "PipelineSprite.h"
#include "Camera.h"
#include "FrameBuffer.h"

class CRenderPassTest : public vk::IRenderPass
{
public:
    CRenderPassTest() : m_pCamera(make<CCamera>()) {}

    CCamera::Ptr getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __createRenderPass();
    void __createGraphicsPipeline();
    void __createDepthResources();
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);

    CPipelineSprite m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    vk::CImage m_DepthImage;

    CCamera::Ptr m_pCamera = nullptr;
};