#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"
#include "FullScreenPoint.h"
#include "PipelineEnvironment.h"

class CRenderPassFullScreen : public IRenderPass
{
public:
    CRenderPassFullScreen() {}

    void setCamera(ptr<CCamera> vCamera) { m_pCamera = vCamera; }
    ptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __createRenderPass();
    void __destroyRenderPass();
    void __createGraphicsPipeline();
    void __createCommandPoolAndBuffers();
    void __createFramebuffers();
    void __createVertexBuffer();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __generateScene();
   
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    CPipelineEnvironment m_Pipeline;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "FullScreen";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    ptr<CCamera> m_pCamera = nullptr;
    std::vector<SFullScreenPointData> m_PointDataSet;
};

