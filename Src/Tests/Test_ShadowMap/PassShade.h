#pragma once
#include "IRenderPass.h"
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

    void setShadowMapInfo(CCamera::CPtr vLightCamera);
    void setScene(const std::vector<ptr<C3DObject>>& vObjectSet);

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
    void __createLightFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);

    CPipelineShade m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertBuffer;
    size_t m_VertexNum = 0;

    vk::CImage m_DepthImage = nullptr;

    ptr<CCamera> m_pCamera = nullptr;

    CCamera::CPtr m_pLightCamera = nullptr;
};

