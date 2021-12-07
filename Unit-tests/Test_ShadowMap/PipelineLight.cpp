#include "PipelineLight.h"

struct SUBOVertLight
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 LightMVP;
};

void CPipelineLight::__createPlaceholderImage()
{
    uint8_t PixelData = 0;
    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = 1;
    ImageInfo.extent.height = 1;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    m_pPlaceholderImage = std::make_shared<vk::CImage>();
    m_pPlaceholderImage->create(m_PhysicalDevice, m_Device, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
}

void CPipelineLight::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferSet[i]->get();
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUBOVertLight);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorImageInfo CombinedSamplerInfo = {};
        CombinedSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        CombinedSamplerInfo.imageView = (m_ShadowMapImageViewSet.empty() ? m_pPlaceholderImage->get() : m_ShadowMapImageViewSet[i]);
        CombinedSamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {CombinedSamplerInfo} }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CPipelineLight::setShadowMapImageViews(std::vector<VkImageView> vShadowMapImageViews)
{
    m_ShadowMapImageViewSet = vShadowMapImageViews;
}

void CPipelineLight::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::mat4 vLightVP, float vShadowMapWidth, float vShadowMapHeight)
{
    SUBOVertLight UBOVert = {};
    UBOVert.Model = vModel;
    UBOVert.View = vView;
    UBOVert.Proj = vProj;
    UBOVert.LightMVP = vLightVP;
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineLight::destroy()
{
    __destroyResources();
    CPipelineBase::destroy();
}

void CPipelineLight::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SLightPointData::getBindingDescription();
    voAttributeSet = SLightPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineLight::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

void CPipelineLight::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVertLight);
    m_VertUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = std::make_shared<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_PhysicalDevice, m_Device, VertBufferSize);
    }

    VkPhysicalDeviceProperties Properties = {};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &Properties);

    VkSamplerCreateInfo SamplerInfo = {};
    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerInfo.magFilter = VK_FILTER_LINEAR;
    SamplerInfo.minFilter = VK_FILTER_LINEAR;
    SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.anisotropyEnable = VK_TRUE;
    SamplerInfo.maxAnisotropy = Properties.limits.maxSamplerAnisotropy;
    SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerInfo.compareEnable = VK_FALSE;
    SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.mipLodBias = 0.0f;
    SamplerInfo.minLod = 0.0f;
    SamplerInfo.maxLod = 0.0f;

    Vulkan::checkError(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler));

    __createPlaceholderImage();
    __updateDescriptorSet();
}

void CPipelineLight::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("CombinedSampler", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
}

void CPipelineLight::__destroyResources()
{
    for (size_t i = 0; i < m_VertUniformBufferSet.size(); ++i)
    {
        m_VertUniformBufferSet[i]->destroy();
    }
    m_VertUniformBufferSet.clear();

    if (m_pPlaceholderImage)
        m_pPlaceholderImage->destroy();

    if (m_TextureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device, m_TextureSampler, nullptr);
        m_TextureSampler = VK_NULL_HANDLE;
    }
}

