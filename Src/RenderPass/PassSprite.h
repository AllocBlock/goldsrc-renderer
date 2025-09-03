#pragma once
#include "RenderPass.h"
#include "PipelineSprite.h"
#include "RenderPass.h"

class CRenderPassSprite : public engine::IRenderPass
{
public:
    CRenderPassSprite();

protected:
    virtual sptr<CPortSet> _createPortSetV() override;
    virtual void _initV() override;
    virtual void _updateV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;

private:
    void __updateUniformBuffer();
    
    CPipelineSprite m_PipelineSprite;
    std::vector<SGoldSrcSprite> m_SpriteSet;
    CRenderInfoDescriptor m_RenderInfoDescriptor;
};
