#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "FullScreenPointData.h"
#include "ImageUtils.h"
#include "InterfaceUI.h"

class CPipelineBloomMerge : public IPipeline
{

public:
    void setInputImages(const std::vector<VkImageView>& vImageViewSet);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;
    virtual void _renderUIV() override;

private:
    void __updateInputImages(const std::vector<VkImageView>& vImageViewSet);
    void __updateUniformBuffer(uint32_t vImageIndex);
    void __initAllDescriptorSet();

    vk::CPointerSet<vk::CUniformBuffer> m_FragUbufferSet;
    vk::CImage m_PlaceholderImage;
    vk::CSampler m_Sampler;

    float m_BloomFactor = 0.5f;
};
