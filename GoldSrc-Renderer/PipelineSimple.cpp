#include "PipelineSimple.h"

size_t CPipelineSimple::MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

struct SUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
};

struct SUniformBufferObjectFrag
{
    alignas(16) glm::vec3 Eye;
};

void CPipelineSimple::updateDescriptorSet(const std::vector<VkImageView>& vTextureSet)
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferPackSet[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo}, {} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferPackSet[i].Buffer;
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUniformBufferObjectFrag);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {FragBufferInfo}, {} }));

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = VK_NULL_HANDLE;
        SamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {SamplerInfo} }));

        //const size_t NumTexture = __getActualTextureNum();
        const size_t NumTexture = vTextureSet.size();

        std::vector<VkDescriptorImageInfo> TexImageInfoSet(CPipelineSimple::MaxTextureNum);
        for (size_t i = 0; i < CPipelineSimple::MaxTextureNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                if (i == 0) // no texture, use default placeholder texture
                {
                    TexImageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    TexImageInfoSet[i].imageView = m_PlaceholderImagePack.ImageView;
                    TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
                }
                else
                {
                    TexImageInfoSet[i] = TexImageInfoSet[0];
                }
            }
            else
            {
                TexImageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                TexImageInfoSet[i].imageView = vTextureSet[i];
                TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
            }
        }
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageInfoSet }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CPipelineSimple::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos)
{
    SUniformBufferObjectVert UBOVert = {};
    UBOVert.Model = vModel;
    UBOVert.View = vView;
    UBOVert.Proj = vProj;

    void* pData;
    Vulkan::checkError(vkMapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory);

    SUniformBufferObjectFrag UBOFrag = {};
    UBOFrag.Eye = vEyePos;

    Vulkan::checkError(vkMapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory);
}

void CPipelineSimple::destroy()
{
    __destroyResources();
    CPipelineBase::destroy();
}

void CPipelineSimple::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SSimplePointData::getBindingDescription();
    voAttributeSet = SSimplePointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineSimple::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

VkPipelineDepthStencilStateCreateInfo CPipelineSimple::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

void CPipelineSimple::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize BufferSize = sizeof(SUniformBufferObjectVert);
    m_VertUniformBufferPackSet.resize(vImageNum);
    m_FragUniformBufferPackSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        Vulkan::createBuffer(m_PhysicalDevice, m_Device, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPackSet[i].Buffer, m_VertUniformBufferPackSet[i].Memory);
        Vulkan::createBuffer(m_PhysicalDevice, m_Device, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBufferPackSet[i].Buffer, m_FragUniformBufferPackSet[i].Memory);
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

    VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
    Vulkan::stageFillImage(m_PhysicalDevice, m_Device, &PixelData, sizeof(uint8_t) * 4, ImageInfo,  m_PlaceholderImagePack.Image, m_PlaceholderImagePack.Memory);
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    m_PlaceholderImagePack.ImageView = Vulkan::createImageView(m_Device, m_PlaceholderImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CPipelineSimple::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Sampler", 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Texture", 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineSimple::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
}

void CPipelineSimple::__destroyResources()
{
    for (size_t i = 0; i < m_VertUniformBufferPackSet.size(); ++i)
    {
        m_VertUniformBufferPackSet[i].destroy(m_Device);
        m_FragUniformBufferPackSet[i].destroy(m_Device);
    }
    m_VertUniformBufferPackSet.clear();
    m_FragUniformBufferPackSet.clear();

    m_PlaceholderImagePack.destroy(m_Device);

    if (m_TextureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device, m_TextureSampler, nullptr);
        m_TextureSampler = VK_NULL_HANDLE;
    }
}