#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "VertexAttributeDescriptor.h"
#include "FullScreenPointData.h"

class CPipelineEdge : public IPipeline
{
public:
    void setInputImage(VkImageView vImageView, size_t vIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __updateUniformBuffer(uint32_t vImageIndex);
    void __initAllDescriptorSet();

    vk::CPointerSet<vk::CUniformBuffer> m_FragUbufferSet;
    vk::CImage::Ptr m_pPlaceholderImage = nullptr;
    vk::CSampler m_Sampler;
};
