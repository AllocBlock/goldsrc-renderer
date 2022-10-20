#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "FullScreenPointData.h"

class CRenderPassFullScreen : public vk::IRenderPass
{
public:
    template <typename T>
    ptr<T> initPipeline()
    {
        VkRenderPass RenderPass = get();
        _ASSERTE(RenderPass != VK_NULL_HANDLE);
        if (m_pPipeline) m_pPipeline->destroy();
        ptr<T> pPipeline = make<T>();
        m_pPipeline = pPipeline;
        m_pPipeline->create(m_AppInfo.pDevice, RenderPass, m_AppInfo.Extent);
        m_pPipeline->setImageNum(m_AppInfo.ImageNum);
        return pPipeline;
    }
    ptr<IPipeline> getPipeline() { return m_pPipeline; }
    
    //VkRenderPass getRenderPass() { return m_RenderPass; }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override; 
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createFramebuffers();
    void __createVertexBuffer();

    void __generateScene();
   
    ptr<IPipeline> m_pPipeline = nullptr;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;
};

