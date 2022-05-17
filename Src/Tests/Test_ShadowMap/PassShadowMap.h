#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "PipelineShadowMap.h"
#include "Camera.h"
#include "Buffer.h"
#include "Image.h"
#include "3DObject.h"

class CRenderPassShadowMap : public vk::IRenderPass
{
public:
    CRenderPassShadowMap() : m_pLightCamera(make<CCamera>())
    {
        
    }

    void setScene(const std::vector<ptr<C3DObject>>& vObjectSet);
    ptr<CCamera> getLightCamera() { return m_pLightCamera; }
    void exportShadowMapToFile(std::string vFileName);

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
    void __createFramebuffer();
    void __createShadowMapImages();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
   
    VkRenderPass m_RenderPassShadowMap = VK_NULL_HANDLE;
    CPipelineShadowMap m_PipelineShadowMap;
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;
    std::vector<vk::CImage::Ptr> m_ShadowMapImageSet;
    ptr<vk::CBuffer> m_pVertBuffer = nullptr;
    std::vector<SShadowMapPointData> m_ShadowMapPointDataSet;

    CCommand m_Command = CCommand();
    std::string m_CommandName = "ShadowMap";
    vk::CImage::Ptr m_pShadowMapDepthImage;

    ptr<CCamera> m_pLightCamera = nullptr;
};

