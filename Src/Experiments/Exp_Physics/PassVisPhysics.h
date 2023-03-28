#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "PhysicsEngine.h"
#include "PipelineVisCollider.h"
#include "PipelineVisCollidePoint.h"

class CRenderPassVisPhysics : public vk::IRenderPass
{
public:
    CRenderPassVisPhysics() = default;

    void setPhysicsEngine(CPhysicsEngine::Ptr vEngine);
    void setCamera(CCamera::Ptr vCamera) { m_pCamera = vCamera; } 

protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor vDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;
    
    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;
    
    virtual void _renderUIV() override;

private:
    void __createGraphicsPipelines();
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    
    CPipelineVisCollider m_PipelineVisCollider;
    CPipelineVisCollidePoint m_PipelineVisCollidePoint;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;

    CPhysicsEngine::Ptr m_pEngine = nullptr;
    CCamera::Ptr m_pCamera = nullptr;

    bool m_ShowCollider = true;
};
