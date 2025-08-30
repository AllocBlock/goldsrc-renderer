#pragma once
#include "Pipeline.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <array>

class CPipelineSkybox : public IPipeline
{
public:
    void setSkyBoxImage(const std::array<ptr<CIOImage>, 6>& vSkyBoxImageSet);
    void updateUniformBuffer(CCamera::CPtr vCamera);
    void recordCommand(CCommandBuffer::Ptr vCommandBuffer);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CImage m_SkyBoxImage;
    vk::CBuffer m_VertexBuffer;
    size_t m_VertexNum = 0;
    vk::CUniformBuffer::Ptr m_pVertUniformBuffer;
    vk::CUniformBuffer::Ptr m_pFragUniformBuffer;
};

