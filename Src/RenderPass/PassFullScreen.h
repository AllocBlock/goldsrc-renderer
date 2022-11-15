#pragma once
#include "RenderPassSingle.h"
#include "FrameBuffer.h"
#include "VertexBuffer.h"
#include "Pipeline.h"
#include "FullScreenPointData.h"

#include <functional>

class CRenderPassFullScreen : public CRenderPassSingle
{
public:
    using PipelineCreateCallback_t = std::function<void()>;

    // create pipeline instance but maybe not valid as renderpass maynot be valid
    template <typename Pipeline_t>
    ptr<Pipeline_t> initPipeline()
    {
        static_assert(std::is_base_of<IPipeline, Pipeline_t>::value, "Full Screen Pass require pipeline type");

        if (m_pPipeline) m_pPipeline->destroy();
        ptr<Pipeline_t> pPipeline = make<Pipeline_t>();

        // FIXME: how to get extent?
        /*if (isValid())
            __createPipeline();*/
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
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override; 
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createPipeline(VkExtent2D vExtent);
    void __createVertexBuffer();

    void __generateScene();
   
    ptr<IPipeline> m_pPipeline = nullptr;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;
    std::vector<PipelineCreateCallback_t> m_PipelineCreateCallbackSet;
};

class CRenderPassFullScreenGeneral : public CRenderPassSingle
{
protected:
    virtual ptr<IPipeline> _initPipelineV() = 0;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) = 0;
    virtual CRenderPassDescriptor _getRenderPassDescV() = 0;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;
    
    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override final
    {
        voExtent = m_pAppInfo->getScreenExtent();
        return true;
    }
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override final
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex),
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override final
    {
        return DefaultClearValueColor;
    }

private:
    virtual void _initV() override final;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override final;
    virtual void _destroyV() override final;
    
private:
    void __createPipeline(VkExtent2D vExtent);
    void __createVertexBuffer();

    void __generateScene();

    ptr<IPipeline> m_pPipeline = nullptr;
    ptr<vk::CVertexBufferTyped<SFullScreenPointData>> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;
};
