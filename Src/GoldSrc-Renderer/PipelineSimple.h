#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <glm/glm.hpp>

class CPipelineSimple : public IPipeline
{
public:
    void setTextures(const vk::CPointerSet<vk::CImage>& vTextureSet);
    void updateUniformBuffer(glm::mat4 vModel, cptr<CCamera> vCamera);

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __destroyResources();

    std::vector<VkImageView> m_TextureSet;
    vk::CSampler m_Sampler;
    sptr<vk::CUniformBuffer> m_pVertUniformBuffer;
    sptr<vk::CUniformBuffer> m_pFragUniformBuffer;
    vk::CImage m_PlaceholderImage;
};