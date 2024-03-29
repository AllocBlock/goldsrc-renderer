#pragma once
#include "PassTempSceneBase.h"
#include "FrameBuffer.h"
#include "PipelineShadowMap.h"
#include "Camera.h"
#include "Image.h"
#include "ShadowMapDefines.h"

class CRenderPassShadowMap : public CRenderPassTempSceneBase<CPipelineShadowMap::SPointData>
{
public:
    CRenderPassShadowMap() : m_pLightCamera(make<CCamera>())
    {  
    }
    
    CCamera::Ptr getLightCamera() { return m_pLightCamera; }
    void exportShadowMapToFile(std::string vFileName);

protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor vDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createDepthResources();
    void __createFramebuffer();
    void __createShadowMapImages();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
   
    CPipelineShadowMap m_PipelineShadowMap;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    vk::CPointerSet<vk::CImage> m_ShadowMapImageSet;
    vk::CImage m_DepthImage;

    CCamera::Ptr m_pLightCamera = nullptr;

    const VkFormat m_ShadowMapFormat = gShadowMapImageFormat;
    const VkExtent2D m_ShadowMapExtent = { gShadowMapSize, gShadowMapSize };
};
