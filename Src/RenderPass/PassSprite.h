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
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override;
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override;
    virtual std::vector<VkClearValue> _getClearValuesV() override;

private:
    void __updateUniformBuffer(uint32_t vImageIndex);
    
    CPipelineSprite m_PipelineSprite;
    std::vector<SGoldSrcSprite> m_SpriteSet;
};
