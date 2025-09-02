#pragma once
#include "RenderPass.h"
#include "Buffer.h"
#include "PipelineOutlineEdge.h"
#include "PipelineOutlineMask.h"
#include "RerecordState.h"

class CRenderPassOutline : public engine::IRenderPass
{
public:
    inline static const std::string Name = "Outline";

    void setHighlightActor(CActor::Ptr vActor)
    {
        m_MaskPipeline.setActor(vActor);
    }

    void removeHighlight()
    {
        m_MaskPipeline.removeObject();
    }

protected:
    virtual void _initV() override;
    virtual CPortSet::Ptr _createPortSetV() override;
    virtual void _updateV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;

private:
    void __createVertexBuffer();

    CRenderInfoDescriptor m_PassMaskDescriptor;
    CRenderInfoDescriptor m_PassOutlineDescriptor;
    vk::CImage::Ptr m_pMaskImage;
    CPipelineMask m_MaskPipeline;
    CPipelineEdge m_EdgePipeline;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;
};
