#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "FullScreenPointData.h"

#include <functional>

class CRenderPassFullScreen : public vk::IRenderPass
{
public:
    using PipelineCreateCallback_t = std::function<void()>;

    // create pipeline instance but maybe not valid as renderpass maynot be valid
    template <typename T>
    ptr<T> initPipeline()
    {
        if (m_pPipeline) m_pPipeline->destroy();
        ptr<T> pPipeline = make<T>();
        if (isValid())
            __createPipeline();
        m_pPipeline = pPipeline;
        return pPipeline;
    }
    ptr<IPipeline> getPipeline() { return m_pPipeline; }

    void hookPipelineCreate(PipelineCreateCallback_t vCallback)
    {
        m_PipelineCreateCallbackSet.emplace_back(vCallback);
    }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override; 
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createPipeline();
    void __createFramebuffers();
    void __createVertexBuffer();

    void __generateScene();
   
    ptr<IPipeline> m_pPipeline = nullptr;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;
    std::vector<PipelineCreateCallback_t> m_PipelineCreateCallbackSet;
};

