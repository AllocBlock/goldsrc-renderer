#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "FullScreenPoint.h"

class CRenderPassFullScreen : public IRenderPass
{
public:
    CRenderPassFullScreen() {}

    template <typename T>
    ptr<T> initPipeline()
    {
        _ASSERTE(m_RenderPass != VK_NULL_HANDLE);
        if (m_pPipeline) m_pPipeline->destroy();
        ptr<T> pPipeline = make<T>();
        m_pPipeline = pPipeline;
        m_pPipeline->create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
        m_pPipeline->setImageNum(m_AppInfo.ImageNum);
        return pPipeline;
    }
    ptr<IPipeline> getPipeline() { return m_pPipeline; }
    
    //VkRenderPass getRenderPass() { return m_RenderPass; }

protected:
    virtual void _initV() override;
    virtual CRenderPassPort _getPortV() override;
    virtual void _recreateV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __createRenderPass();
    void __destroyRenderPass();
    void __createCommandPoolAndBuffers();
    void __createFramebuffers();
    void __createVertexBuffer();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __generateScene();
   
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    ptr<IPipeline> m_pPipeline = nullptr;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "FullScreen";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;
};

