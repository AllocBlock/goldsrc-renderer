#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"

class CPipelineBloomLuminance : public IPipeline
{
public:
    void setInputImage(VkImageView vImageView);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;
    virtual void _renderUIV() override;

private:
    void __initAllDescriptorSet();

    vk::CImage m_PlaceholderImage;
    vk::CSampler m_Sampler;
};
