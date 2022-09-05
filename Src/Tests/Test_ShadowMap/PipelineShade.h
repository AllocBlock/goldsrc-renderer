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

    using PointData_t = SLightPointData;
    _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
        Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
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
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CImage m_PlaceholderImage;
    std::vector<VkImageView> m_ShadowMapImageViewSet;
};

