#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "FullScreenPointData.h"
#include "ImageUtils.h"
#include "InterfaceUI.h"
#include "RenderPassPort.h"

class CPipelineBloomBlur : public IPipeline
{
protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;
    virtual void _renderUIV() override;

private:
    void __updateInputImage(VkImageView vImageView, size_t vIndex);
    void __updateUniformBuffer(uint32_t vImageIndex);
    void __initAllDescriptorSet();

    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
    vk::CImage m_PlaceholderImage;
    vk::CSampler m_Sampler;

    float m_FilterSize = 5.0f;
};
