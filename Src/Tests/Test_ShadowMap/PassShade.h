#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "PipelineShade.h"
#include "Camera.h"
#include "Buffer.h"
#include "Image.h"
#include "3DObject.h"

class CRenderPassShade : public vk::IRenderPass
{
public:
    CRenderPassShade() : m_pCamera(make<CCamera>())
    {
    }

    ptr<CCamera> getCamera() { return m_pCamera; }

    void setShadowMapInfo(std::vector<VkImageView> vShadowMapImageViews, CCamera
        ::CPtr vLightCamera);
    void setScene(const std::vector<ptr<C3DObject>>& vObjectSet);

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
    void __createLightFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);

    CPipelineShade m_Pipeline;
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertBuffer;
    std::vector<SLightPointData> m_PointDataSet;

    CCommand m_Command = CCommand();
    std::string m_CommandName = "Shade";
    vk::CImage::Ptr m_pDepthImage = nullptr;

    ptr<CCamera> m_pCamera = nullptr;

    std::vector<VkImageView> m_ShadowMapImageViewSet;
    CCamera::CPtr m_pLightCamera = nullptr;
};

