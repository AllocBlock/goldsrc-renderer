#pragma once
#include "RenderPassSingle.h"
#include "Buffer.h"
#include "PipelineOutlineEdge.h"

class CRenderPassOutlineEdge : public CRenderPassSingle
{
protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
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
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex)
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColor;
    }

private:
    void __createVertexBuffer();

    CPipelineEdge m_Pipeline;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<CPipelineEdge::SPointData> m_PointDataSet;
};
