#pragma once
#include "RendererBase.h"
#include "PipelineSprite.h"
#include "Camera.h"

class CRendererTest : public CRendererBase
{
public:
    CRendererTest() : m_pCamera(std::make_shared<CCamera>()) {}

    std::shared_ptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __createRenderPass();
    void __destroyRenderPass();
    void __createGraphicsPipeline();
    void __createCommandPoolAndBuffers();
    void __createDepthResources();
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    CPipelineSprite m_Pipeline;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Test";
    std::vector<VkFramebuffer> m_FramebufferSet;
    Vulkan::SImagePack m_DepthImagePack;

    std::shared_ptr<CCamera> m_pCamera = nullptr;
};