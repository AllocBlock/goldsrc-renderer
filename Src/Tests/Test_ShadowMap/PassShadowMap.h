#pragma once
#include "IRenderPass.h"
#include "FrameBuffer.h"
#include "PipelineShadowMap.h"
#include "Camera.h"
#include "Buffer.h"
#include "Image.h"
#include "3DObject.h"
#include "ShadowMapDefines.h"

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
    virtual SPortDescriptor _getPortDescV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __createRenderPass();
    void __createGraphicsPipeline();
    void __createFramebuffer();
    void __createShadowMapImages();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
   
    CPipelineShadowMap m_PipelineShadowMap;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    vk::CPointerSet<vk::CImage> m_ShadowMapImageSet;
    ptr<vk::CBuffer> m_pVertBuffer = nullptr;
    size_t m_VertexNum = 0;

    ptr<CCamera> m_pLightCamera = nullptr;

    const VkFormat m_ShadowMapFormat = gShadowMapImageFormat;
    const VkExtent2D m_ShadowMapExtent = { gShadowMapSize, gShadowMapSize };
};

