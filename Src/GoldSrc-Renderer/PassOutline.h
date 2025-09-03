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

    void setHighlightActor(sptr<CActor> vActor)
    {
        m_MaskPipeline.setActor(vActor);
    }

    void removeHighlight()
    {
        m_MaskPipeline.removeObject();
    }

protected:
    virtual void _initV() override;
    virtual sptr<CPortSet> _createPortSetV() override;
    virtual void _updateV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;

private:
    void __createVertexBuffer();

    CRenderInfoDescriptor m_PassMaskDescriptor;
    CRenderInfoDescriptor m_PassOutlineDescriptor;
    sptr<vk::CImage> m_pMaskImage;
    CPipelineMask m_MaskPipeline;
    CPipelineEdge m_EdgePipeline;
    sptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<SFullScreenPointData> m_PointDataSet;
};
