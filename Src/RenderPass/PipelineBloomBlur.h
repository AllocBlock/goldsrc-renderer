#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"

class CPipelineBloomBlur : public IPipeline
{
public:
    void setInputImage(VkImageView vImageView);
    void updateUniformBuffer(uint32_t vFilterSize);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;
    virtual void _renderUIV() override;

private:
    void __initAllDescriptorSet();

    sptr<vk::CUniformBuffer> m_pFragUniformBuffer;
    vk::CImage m_PlaceholderImage;
    vk::CSampler m_Sampler;
};
