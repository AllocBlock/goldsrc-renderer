#pragma once
#include "RendererBase.h"
#include "FrameBuffer.h"
#include "PipelineShadowMap.h"
#include "PipelineLight.h"
#include "Camera.h"
#include "Buffer.h"
#include "Image.h"

class CRendererTest : public CRendererBase
{
public:
    CRendererTest() : m_pCamera(std::make_shared<CCamera>()),
        m_pLightCamera(std::make_shared<CCamera>())
    {
        
    }

    std::shared_ptr<CCamera> getCamera() { return m_pCamera; }
    void exportShadowMapToFile(std::string vFileName);

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    VkRenderPass __createRenderPassGeneral(VkAttachmentDescription vColorAttachment, VkAttachmentDescription vDepthAttachment);
    void __createRenderPassShadowMap();
    void __createRenderPassLighting();
    void __destroyRenderPasses();
    void __createGraphicsPipeline();
    void __createCommandPoolAndBuffers();
    void __createDepthResources();
    void __createLightFramebuffers();
    void __createShadowMapFramebuffers();
    void __createVertexBuffer();
    void __createShadowMapImages();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __generateScene();
    void __appendCube(glm::vec3 vCenter, float vSize);
    void __recordShadowMapRenderPass(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex);
    void __recordLightRenderPass(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex);
   
    VkRenderPass m_RenderPassShadowMap = VK_NULL_HANDLE;
    CPipelineShadowMap m_PipelineShadowMap;
    std::vector<std::shared_ptr<vk::CFrameBuffer>> m_ShadowFramebufferSet;
    std::vector<std::shared_ptr<vk::CImage>> m_ShadowMapImageSet;
    std::shared_ptr<vk::CBuffer> m_ShadowMapVertBuffer;
    std::vector<SShadowMapPointData> m_ShadowMapPointDataSet;

    VkRenderPass m_RenderPassLight = VK_NULL_HANDLE;
    CPipelineLight m_PipelineLight;
    std::vector<std::shared_ptr<vk::CFrameBuffer>> m_LightFramebufferSet;
    std::shared_ptr<vk::CBuffer> m_pLightVertBuffer;
    std::vector<SLightPointData> m_LightPointDataSet;

    CCommand m_Command = CCommand();
    std::string m_CommandName = "Test";
    std::shared_ptr<vk::CImage> m_pShadowMapDepthImage;
    std::shared_ptr<vk::CImage> m_pLightDepthImage;

    std::shared_ptr<CCamera> m_pCamera = nullptr;
    std::shared_ptr<CCamera> m_pLightCamera = nullptr;

    VkFormat m_ShadowMapImageFormat = VkFormat::VK_FORMAT_R32_SFLOAT;
};

