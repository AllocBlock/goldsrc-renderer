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
    void setLightmapState(VkCommandBuffer vCommandBuffer, bool vEnable);
    void setOpacity(VkCommandBuffer vCommandBuffer, float vOpacity);
    void updateDescriptorSet(const vk::CPointerSet<vk::CImage>& vTextureSet, VkImageView vLightmap);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, CCamera::CPtr vCamera);

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initPushConstantV(VkCommandBuffer vCommandBuffer) override;
    virtual void _destroyV() override;

    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) = 0;

private:
    void __destroyResources();
    void __updatePushConstant(VkCommandBuffer vCommandBuffer, bool vEnableLightmap, float vOpacity);
    
    bool m_EnableLightmap = false;
    float m_Opacity = 1.0f;

    vk::CSampler m_Sampler;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
    vk::CImage m_PlaceholderImage;
};
