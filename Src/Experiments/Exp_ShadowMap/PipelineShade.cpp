#include "PipelineShade.h"
#include "ImageUtils.h"

namespace
{
    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
        alignas(16) glm::mat4 Model;
        alignas(16) glm::mat4 LightVP;
    };
}

void CPipelineShade::setShadowMapImageViews(std::vector<VkImageView> vShadowMapImageViews)
{
    m_ShadowMapImageViewSet = vShadowMapImageViews;
    __updateDescriptorSet();
}

void CPipelineShade::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteImageAndSampler(1, (m_ShadowMapImageViewSet.empty() ? m_PlaceholderImage : m_ShadowMapImageViewSet[i]), m_Sampler);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineShade::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera, CCamera::CPtr vLightCamera, uint32_t vShadowMapSize)
{
    SUBOVert UBOVert = {};
    UBOVert.Model = glm::mat4(1.0f);
    UBOVert.View = vCamera->getViewMat();
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.LightVP = vLightCamera->getViewProjMat();
    // FIXME: is shadowmap size needed?
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineShade::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("CombinedSampler", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineShade::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("shaders/shaderVert.spv");
    Descriptor.setFragShaderPath("shaders/shaderFrag.spv");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setRasterFrontFace(VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE);

    return Descriptor;
}

void CPipelineShade::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_VertUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    ImageUtils::createPlaceholderImage(m_PlaceholderImage, m_pDevice);
    __updateDescriptorSet();
}

void CPipelineShade::_destroyV()
{
    __destroyResources();
}

void CPipelineShade::__destroyResources()
{
    m_VertUniformBufferSet.destroyAndClearAll();
    m_PlaceholderImage.destroy();
    m_Sampler.destroy();
}
