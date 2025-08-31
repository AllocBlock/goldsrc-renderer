#pragma once
#include "RenderPass.h"
#include "PipelineSprite.h"
#include "RenderPass.h"

class CRenderPassSprite : public engine::IRenderPass
{
public:
    CRenderPassSprite();

protected:
    virtual CPortSet::Ptr _createPortSetV() override;
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
