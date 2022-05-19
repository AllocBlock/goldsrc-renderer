#pragma once
#include "IRenderPass.h"
#include "PipelineSprite.h"
#include "Camera.h"
#include "FrameBuffer.h"

class CRendererTest : public vk::IRenderPass
{
public:
    CRendererTest() : m_pCamera(make<CCamera>()) {}

    ptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual CRenderPassPort _getPortV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __createRenderPass();
    void __createGraphicsPipeline();
    void __createCommandPoolAndBuffers();
    void __createDepthResources();
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);

    CPipelineSprite m_Pipeline;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Test";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;
    vk::CImage::Ptr m_pDepthImage;

    ptr<CCamera> m_pCamera = nullptr;
};