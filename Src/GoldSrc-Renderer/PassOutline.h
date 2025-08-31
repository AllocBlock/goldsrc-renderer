#pragma once
#include "RenderPass.h"
#include "Buffer.h"
#include "PipelineOutlineEdge.h"
#include "PipelineOutlineMask.h"
#include "RerecordState.h"

// FIXME: should not use subpass like this
// Quote: Image subresources used as attachments must not be accessed in any other way for the duration of a render pass instance.
class CRenderPassOutline : public engine::IRenderPass
{
public:
    inline static const std::string Name = "Outline";

    void setHighlightActor(CActor::Ptr vActor)
    {
        m_MaskPipeline.setActor(vActor);
        m_pRerecord->requestRecordForAll();
    }

    void removeHighlight()
    {
        m_MaskPipeline.removeObject();
        m_pRerecord->requestRecordForAll();
    }

protected:
    virtual void _initV() override;
    virtual CPortSet::Ptr _createPortSetV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        return _dumpInputPortExtent("Main", voExtent);
    }
    virtual std::vector<VkImageView> _getAttachmentsV() override
    {
        _ASSERTE(m_MaskImage.isValid());
        return
        {
            m_MaskImage,
            m_pPortSet->getOutputPort("Main")->getImageV(),
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

    vk::CImage m_MaskImage;
    CPipelineMask m_MaskPipeline;
    CPipelineEdge m_EdgePipeline;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;

    // rerecord control
    CRerecordState::Ptr m_pRerecord = nullptr;
};
