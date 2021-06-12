#include "PipelineDepthTest.h"

size_t CPipelineDepthTest::MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

struct SPushConstant
{
    VkBool32 UseLightmap = VK_FALSE;
    float Opacity = 1.0;
};

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

void CPipelineDepthTest::updateDescriptorSet(const std::vector<VkImageView>& vTextureSet, VkImageView vLightmap)
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

        std::vector<VkDescriptorImageInfo> TexImageInfoSet(CPipelineDepthTest::MaxTextureNum);
        for (size_t i = 0; i < CPipelineDepthTest::MaxTextureNum; ++i)
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

        VkDescriptorImageInfo LightmapImageInfo = {};
        LightmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        LightmapImageInfo.imageView = vLightmap == VK_NULL_HANDLE ? m_PlaceholderImagePack.ImageView : vLightmap;
        LightmapImageInfo.sampler = VK_NULL_HANDLE;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {LightmapImageInfo} }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CPipelineDepthTest::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos)
{
    SUniformBufferObjectVert UBOVert = {};
    UBOVert.Model = vModel;
    UBOVert.View = vView;
    UBOVert.Proj = vProj;

    void* pData;
    ck(vkMapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory);

    SUniformBufferObjectFrag UBOFrag = {};
    UBOFrag.Eye = vEyePos;

    ck(vkMapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory);
}

void CPipelineDepthTest::setLightmapState(VkCommandBuffer vCommandBuffer, bool vEnable)
{
    if (m_EnableLightmap == vEnable) return;
    else
    {
        m_EnableLightmap = vEnable;
        __updatePushConstant(vCommandBuffer, vEnable, m_Opacity);
    }
}

void CPipelineDepthTest::setOpacity(VkCommandBuffer vCommandBuffer, float vOpacity)
{
    if (m_Opacity == vOpacity) return;
    else
    {
        m_Opacity = vOpacity;
        __updatePushConstant(vCommandBuffer, m_EnableLightmap, vOpacity);
    }
}

void CPipelineDepthTest::destroy()
{
    __destroyResources();
    CPipelineBase::destroy();
}

void CPipelineDepthTest::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SGoldSrcPointData::getBindingDescription();
    voAttributeSet = SGoldSrcPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineDepthTest::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

VkPipelineDepthStencilStateCreateInfo CPipelineDepthTest::_getDepthStencilInfoV()
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

std::vector<VkDynamicState> CPipelineDepthTest::_getEnabledDynamicSetV()
{
    return { VK_DYNAMIC_STATE_DEPTH_BIAS };
}

std::vector<VkPushConstantRange> CPipelineDepthTest::_getPushConstantRangeSetV()
{
    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    return { PushConstantInfo };
}

void CPipelineDepthTest::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize BufferSize = sizeof(SUniformBufferObjectVert);
    m_VertUniformBufferPackSet.resize(vImageNum);
    m_FragUniformBufferPackSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        Common::createBuffer(m_PhysicalDevice, m_Device, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPackSet[i].Buffer, m_VertUniformBufferPackSet[i].Memory);
        Common::createBuffer(m_PhysicalDevice, m_Device, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBufferPackSet[i].Buffer, m_FragUniformBufferPackSet[i].Memory);
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

    ck(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler));

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

    VkCommandBuffer CommandBuffer = Common::beginSingleTimeBuffer();
    Common::stageFillImage(m_PhysicalDevice, m_Device, &PixelData, sizeof(uint8_t), ImageInfo,  m_PlaceholderImagePack.Image, m_PlaceholderImagePack.Memory);
    Common::endSingleTimeBuffer(CommandBuffer);

    m_PlaceholderImagePack.ImageView = Common::createImageView(m_Device, m_PlaceholderImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CPipelineDepthTest::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Sampler", 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Texture", 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, CPipelineDepthTest::MaxTextureNum, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Lightmap", 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
}

void CPipelineDepthTest::_initPushConstantV(VkCommandBuffer vCommandBuffer)
{
    __updatePushConstant(vCommandBuffer, m_EnableLightmap, m_Opacity);
}

void CPipelineDepthTest::__destroyResources()
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

void CPipelineDepthTest::__updatePushConstant(VkCommandBuffer vCommandBuffer, bool vEnableLightmap, float vOpacity)
{
    SPushConstant PushConstant;
    PushConstant.UseLightmap = vEnableLightmap;
    PushConstant.Opacity = vOpacity;
    pushConstant(vCommandBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
}