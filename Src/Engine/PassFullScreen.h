#pragma once
#include "RenderPass.h"
#include "VertexBuffer.h"
#include "Pipeline.h"
#include "FullScreenPointData.h"

class CRenderPassFullScreen : public engine::IRenderPass
{
protected:
    virtual CPortSet::Ptr _createPortSetV() = 0;
    virtual void _initV() override;
    virtual void _destroyV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() = 0;

    void _bindVertexBuffer(CCommandBuffer::Ptr vCommandBuffer);
    void _drawFullScreen(CCommandBuffer::Ptr vCommandBuffer);
    
private:
    void __createVertexBuffer();
    void __generateScene();

    ptr<vk::CVertexBufferTyped<SFullScreenPointData>> m_pVertexBuffer = nullptr;
    std::vector<SFullScreenPointData> m_PointDataSet;
};
