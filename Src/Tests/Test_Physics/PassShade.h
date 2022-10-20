#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "PipelineShade.h"
#include "Camera.h"
#include "Buffer.h"
#include "Image.h"
#include "TempScene.h"

class CRenderPassShade : public vk::IRenderPass
{
public:
    CRenderPassShade() = default;

    void setScene(CTempScene::Ptr vScene);
    void setCamera(ptr<CCamera> vCamera) { m_pCamera = vCamera; } 

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;
    
    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createGraphicsPipelines();
    void __createDepthResources();
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);

    CPipelineShade m_PipelineShade;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;

    CTempScene::Ptr m_pScene = nullptr;
    ptr<vk::CBuffer> m_pVertBuffer;
    std::vector<SActorDataInfo> m_ActorDataPositionSet;
    size_t m_VertexNum = 0;

    vk::CImage m_DepthImage;

    ptr<CCamera> m_pCamera = nullptr;
};

