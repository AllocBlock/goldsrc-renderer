#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <glm/glm.hpp>

class CPipelineGoldSrc : public IPipeline
{
public:
    void setLightmapState(CCommandBuffer::Ptr vCommandBuffer, bool vEnable);
    void setOpacity(CCommandBuffer::Ptr vCommandBuffer, float vOpacity);
    void setTextures(const vk::CPointerSet<vk::CImage>& vTextureSet);
    void setLightmap(VkImageView vLightmap);
    void clearResources();
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, CCamera::CPtr vCamera);

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _initPushConstantV(CCommandBuffer::Ptr vCommandBuffer) override;
    virtual void _destroyV() override;

    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) = 0;

private:
    void __destroyResources();
    void __updateDescriptorSet();
    void __updatePushConstant(CCommandBuffer::Ptr vCommandBuffer, bool vEnableLightmap, float vOpacity);
    
    bool m_EnableLightmap = false;
    float m_Opacity = 1.0f;
    std::vector<VkImageView> m_TextureSet;
    VkImageView m_LightmapTexture = VK_NULL_HANDLE;

    vk::CSampler m_Sampler;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
    vk::CImage m_PlaceholderImage;
};
