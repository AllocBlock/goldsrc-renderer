#pragma once
#include "Camera.h"
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"

class CPipelineSSAO : public IPipeline
{
public:
    void setDepthImage(VkImageView vImageView, size_t vIndex);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::Ptr vCamera);

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

    vk::CPointerSet<vk::CUniformBuffer> m_FragUBSet;
    float m_Strength = 1.0f;
    int m_SampleNum = 32;
    float m_SampleRadius = 0.1f;
    float m_Time = 0.0f;
};
