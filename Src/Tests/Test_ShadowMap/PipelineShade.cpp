#include "PipelineShade.h"
#include "Function.h"

struct SUBOVertLight
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 LightVP;
};

void CPipelineShade::setShadowMapImageViews(std::vector<VkImageView> vShadowMapImageViews)
{
    m_ShadowMapImageViewSet = vShadowMapImageViews;
    __updateDescriptorSet();
}

void CPipelineShade::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_VertUniformBufferSet[i]);
        WriteInfo.addWriteImageAndSampler(1, (m_ShadowMapImageViewSet.empty() ? *m_pPlaceholderImage : m_ShadowMapImageViewSet[i]), m_Sampler);
        m_Descriptor.update(i, WriteInfo);
    }
}

void CPipelineShade::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera, CCamera::CPtr vLightCamera, uint32_t vShadowMapSize)
{
    SUBOVertLight UBOVert = {};
    UBOVert.Model = glm::mat4(1.0f);
    UBOVert.View = vCamera->getViewMat();
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.LightVP = vLightCamera->getViewProjMat();
    // FIXME: is shadowmap size needed?
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineShade::destroy()
{
    __destroyResources();
    IPipeline::destroy();
}

void CPipelineShade::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SLightPointData::getBindingDescription();
    voAttributeSet = SLightPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineShade::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

void CPipelineShade::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVertLight);
    m_VertUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = make<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
    }

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    m_pPlaceholderImage = Function::createPlaceholderImage(m_pDevice);
    __updateDescriptorSet();
}

void CPipelineShade::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("CombinedSampler", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineShade::__destroyResources()
{
    for (size_t i = 0; i < m_VertUniformBufferSet.size(); ++i)
    {
        m_VertUniformBufferSet[i]->destroy();
    }
    m_VertUniformBufferSet.clear();

    if (m_pPlaceholderImage)
        m_pPlaceholderImage->destroy();

    m_Sampler.destroy();
}

