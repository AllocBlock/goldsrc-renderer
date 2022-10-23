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
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
    {
        if (m_VertexBuffer.isValid() && m_SkyBoxImage.isValid())
        {
            VkBuffer VertBuffer = m_VertexBuffer;
            const VkDeviceSize Offsets[] = { 0 };
            bind(vCommandBuffer, vImageIndex);
            vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &VertBuffer, Offsets);
            vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
        }
    }

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CImage m_SkyBoxImage;
    vk::CBuffer m_VertexBuffer;
    size_t m_VertexNum = 0;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
};

