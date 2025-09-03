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
    void setLightmapState(sptr<CCommandBuffer> vCommandBuffer, bool vEnable);
    void setOpacity(sptr<CCommandBuffer> vCommandBuffer, float vOpacity);
    void setTextures(const vk::CPointerSet<vk::CImage>& vTextureSet);
    void setLightmap(VkImageView vLightmap);
    void clearResources();
    void updateUniformBuffer(glm::mat4 vModel, cptr<CCamera> vCamera);

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _initPushConstantV(sptr<CCommandBuffer> vCommandBuffer) override;
    virtual void _destroyV() override;

    virtual void _dumpExtraPipelineDescriptionV(CPipelineDescriptor& vioDesc) = 0;

private:
    void __destroyResources();
    void __updateDescriptorSet();
    void __updatePushConstant(sptr<CCommandBuffer> vCommandBuffer, bool vEnableLightmap, float vOpacity);
    
    bool m_EnableLightmap = false;
    float m_Opacity = 1.0f;
    std::vector<VkImageView> m_TextureSet;
    VkImageView m_LightmapTexture = VK_NULL_HANDLE;

    vk::CSampler m_Sampler;
    sptr<vk::CUniformBuffer> m_pVertUniformBuffer;
    sptr<vk::CUniformBuffer> m_pFragUniformBuffer;
    vk::CImage m_PlaceholderImage;
};
