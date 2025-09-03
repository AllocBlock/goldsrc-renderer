#pragma once
#include "PassTempSceneBase.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Image.h"
#include "PipelineShade.h"

class CRenderPassShade : public CRenderPassTempSceneBase<CPipelineShade::SPointData>
{
public:
    CRenderPassShade() : m_pCamera(make<CCamera>())
    {
    }

    sptr<CCamera> getCamera() { return m_pCamera; }

    void setShadowMapInfo(cptr<CCamera> vLightCamera);

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
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __updateShadowMapImages();

    CPipelineShade m_PipelineShade;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;

    vk::CImage m_DepthImage;

    sptr<CCamera> m_pCamera = nullptr;
    cptr<CCamera> m_pLightCamera = nullptr;
};
