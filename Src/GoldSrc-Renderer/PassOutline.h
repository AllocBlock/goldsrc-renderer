#pragma once
#include "RenderPassSingleFrameBuffer.h"
#include "Buffer.h"
#include "PipelineOutlineEdge.h"
#include "PipelineOutlineMask.h"
#include "DynamicResourceManager.h"
#include "RerecordState.h"

// FIXME: should not use subpass like this
// Quote: Image subresources used as attachments must not be accessed in any other way for the duration of a render pass instance.
class CRenderPassOutline : public CRenderPassSingleFrameBuffer
{
public:
    inline static const std::string Name = "Outline";

    void setHighlightActor(CActor::Ptr vActor)
    {
        m_MaskPipelineCreator.get().setActor(vActor);
        m_pRerecord->requestRecordForAll();
    }

    void removeHighlight()
    {
        m_MaskPipelineCreator.get().removeObject();
        m_pRerecord->requestRecordForAll();
    }

protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        return _dumpInputPortExtent("Main", voExtent);
    }
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_MaskImageCreator.getImageViewV(0),
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex),
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        VkClearValue Value;
        Value.color = { 0.0f, 0.0f, 0.0f, 1.0f };

        return { Value, Value };
    }

private:
    void __createVertexBuffer();

    CDynamicTextureCreator m_MaskImageCreator;
    CDynamicPipeline<CPipelineMask> m_MaskPipelineCreator;
    CDynamicPipeline<CPipelineEdge> m_EdgePipelineCreator;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<CPipelineEdge::SPointData> m_PointDataSet;

    // rerecord control
    CRerecordState::Ptr m_pRerecord = nullptr;
};
