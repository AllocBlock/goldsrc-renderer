#pragma once
#include "RenderPass.h"
#include "VertexBuffer.h"
#include "Pipeline.h"
#include "FullScreenPointData.h"

class CRenderPassFullScreen : public engine::IRenderPass
{
protected:
    virtual sptr<CPortSet> _createPortSetV() = 0;
    virtual void _initV() override;
    virtual void _destroyV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() = 0;

    void _bindVertexBuffer(sptr<CCommandBuffer> vCommandBuffer);
    void _drawFullScreen(sptr<CCommandBuffer> vCommandBuffer);
    
private:
    void __createVertexBuffer();
    void __generateScene();

    sptr<vk::CVertexBufferTyped<SFullScreenPointData>> m_pVertexBuffer = nullptr;
    std::vector<SFullScreenPointData> m_PointDataSet;
};
