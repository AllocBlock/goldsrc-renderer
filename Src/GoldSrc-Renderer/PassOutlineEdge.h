#pragma once
#include "RenderPassSingleFrameBuffer.h"
#include "Buffer.h"
#include "PipelineOutlineEdge.h"
#include "DynamicResourceManager.h"

class CRenderPassOutlineEdge : public CRenderPassSingleFrameBuffer
{
public:
    virtual std::string getNameV() const override { return "Outline Edge"; }
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

    CDynamicPipeline<CPipelineEdge> m_PipelineCreator;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<CPipelineEdge::SPointData> m_PointDataSet;
};
