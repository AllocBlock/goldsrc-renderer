#pragma once
#include "IPipeline.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "VertexAttributeDescriptor.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <array>

struct SLightPointData
{
    glm::vec3 Pos;
    glm::vec3 Normal;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SLightPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SLightPointData, Pos));
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SLightPointData, Normal));
        return Descriptor.generate();
    }
};

class CPipelineShade : public IPipeline
{
public:
    void setShadowMapImageViews(std::vector<VkImageView> vShadowMapImageViews);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera, CCamera::CPtr vLightCamera, uint32_t vShadowMapSize);
    void destroy();

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/shaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/shaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __updateDescriptorSet();
    void __destroyResources();

    vk::CSampler m_Sampler;
    std::vector<ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
    vk::CImage::Ptr m_pPlaceholderImage;
    std::vector<VkImageView> m_ShadowMapImageViewSet;
};

