#pragma once
#include "RenderPassSingleFrameBuffer.h"
#include "VertexBuffer.h"
#include "Pipeline.h"
#include "FullScreenPointData.h"

class CRenderPassFullScreen : public CRenderPassSingleFrameBuffer
{
protected:
    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override final
    {
        voExtent = m_ScreenExtent;
        return true;
    }

    virtual std::vector<VkImageView> _getAttachmentsV() override
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV(),
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColor;
    }

protected:
    virtual void _initV() override;
    virtual void _destroyV() override;

    void _bindVertexBuffer(CCommandBuffer::Ptr vCommandBuffer);
    void _drawFullScreen(CCommandBuffer::Ptr vCommandBuffer);
    
private:
    void __createVertexBuffer();
    void __generateScene();

    ptr<vk::CVertexBufferTyped<SFullScreenPointData>> m_pVertexBuffer = nullptr;
    std::vector<SFullScreenPointData> m_PointDataSet;
};
