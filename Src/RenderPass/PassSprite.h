#pragma once
#include "RenderPass.h"
#include "PipelineSprite.h"
#include "RenderPassSingleFrameBuffer.h"

class CRenderPassSprite : public CRenderPassSingleFrameBuffer
{
public:
    CRenderPassSprite();

protected:
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _initV() override;
    virtual void _updateV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override;
    virtual std::vector<VkImageView> _getAttachmentsV() override;
    virtual std::vector<VkClearValue> _getClearValuesV() override;

private:
    void __updateUniformBuffer();
    
    CPipelineSprite m_PipelineSprite;
    std::vector<SGoldSrcSprite> m_SpriteSet;
};
