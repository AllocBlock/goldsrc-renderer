#pragma once
#include "RenderPass.h"
#include "PipelineSprite.h"
#include "Camera.h"
#include "FrameBuffer.h"

class CRenderPassSprite : public vk::IRenderPass
{
public:
    CRenderPassSprite() : m_pCamera(make<CCamera>()) {}

    CCamera::Ptr getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createGraphicsPipeline();
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);

    CPipelineSprite m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;

    CCamera::Ptr m_pCamera = nullptr;
};