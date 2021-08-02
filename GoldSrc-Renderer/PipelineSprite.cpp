#include "PipelineSprite.h"
#include "Vulkan.h"

#include <glm/ext/matrix_transform.hpp>

const size_t CPipelineSprite::MaxSpriteNum = 2048;

struct SPositionUVPointData
{
    glm::vec3 Pos;
    glm::vec2 TexCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SPositionUVPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptionSet(2);

        AttributeDescriptionSet[0].binding = 0;
        AttributeDescriptionSet[0].location = 0;
        AttributeDescriptionSet[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[0].offset = offsetof(SPositionUVPointData, Pos);

        AttributeDescriptionSet[1].binding = 0;
        AttributeDescriptionSet[1].location = 1;
        AttributeDescriptionSet[1].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptionSet[1].offset = offsetof(SPositionUVPointData, TexCoord);

        return AttributeDescriptionSet;
    }
};

struct SPushConstant
{
    alignas(16) glm::vec3 Origin;
    alignas(16) uint32_t TexIndex;
};

struct SSkyUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::vec3 EyePosition;
};

void CPipelineSprite::destroy()
{
    if (m_Device == VK_NULL_HANDLE) return;

    if (m_TextureSampler != VK_NULL_HANDLE)
        vkDestroySampler(m_Device, m_TextureSampler, nullptr);

    for(auto& ImagePack : m_SpriteImagePackSet)
        ImagePack.destroy(m_Device);
    m_SpriteImagePackSet.clear();

    m_VertexDataPack.destroy(m_Device);
    for (size_t i = 0; i < m_VertUniformBufferPacks.size(); ++i)
    {
        m_VertUniformBufferPacks[i].destroy(m_Device);
    }
    m_VertUniformBufferPacks.clear();

    CPipelineBase::destroy();
}

void CPipelineSprite::setSprites(const std::vector<SGoldSrcSprite>& vSpriteImageSet)
{
    _ASSERTE(vSpriteImageSet.size() <= CPipelineSprite::MaxSpriteNum);
    // Ϊͼ�괴��vkimage
    for (auto& ImagePack : m_SpriteImagePackSet)
        ImagePack.destroy(m_Device);
    m_SpriteImagePackSet.resize(vSpriteImageSet.size());
    for (size_t i = 0; i < vSpriteImageSet.size(); ++i)
    {
        __createImageFromIOImage(vSpriteImageSet[i].pImage, m_SpriteImagePackSet[i]);
        m_SpriteSequence.emplace_back(std::make_pair(vSpriteImageSet[i].Position, i));
    }
    
    __updateDescriptorSet();
}

void CPipelineSprite::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos)
{
    SSkyUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = vProj;
    UBOVert.View = vView;
    UBOVert.EyePosition = vEyePos;

    void* pData;
    Vulkan::checkError(vkMapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory);
}

void CPipelineSprite::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    if (m_VertexDataPack.isValid())
    {
        const VkDeviceSize Offsets[] = { 0 };
        bind(vCommandBuffer, vImageIndex);
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &m_VertexDataPack.Buffer, Offsets);
        for (auto &[Origin, TexIndex] : m_SpriteSequence)
        {
            SPushConstant Data = { Origin, TexIndex };
            vkCmdPushConstants(vCommandBuffer, m_PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Data), &Data);
            vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
        }
        
    }
}

void CPipelineSprite::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SPositionUVPointData::getBindingDescription();
    voAttributeSet = SPositionUVPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineSprite::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

VkPipelineDepthStencilStateCreateInfo CPipelineSprite::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_FALSE;
    DepthStencilInfo.depthWriteEnable = VK_FALSE;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

std::vector<VkPushConstantRange> CPipelineSprite::_getPushConstantRangeSetV()
{
    VkPushConstantRange PushConstantVertInfo = {};
    PushConstantVertInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    PushConstantVertInfo.offset = 0;
    PushConstantVertInfo.size = sizeof(SPushConstant);

    VkPushConstantRange PushConstantFragInfo = {};
    PushConstantFragInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantFragInfo.offset = 0;
    PushConstantFragInfo.size = sizeof(SPushConstant);

    return { PushConstantVertInfo, PushConstantFragInfo };
}

void CPipelineSprite::_initPushConstantV(VkCommandBuffer vCommandBuffer)
{
    SPushConstant Data = { {0.0, 0.0, 0.0}, 0 };
    vkCmdPushConstants(vCommandBuffer, m_PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Data), &Data);
}

void CPipelineSprite::_createResourceV(size_t vImageNum)
{
    // create unit square facing positive x-axis
    const std::vector<SPositionUVPointData> PointData =
    {
        {{ 1.0,  1.0, 1.0 }, {0.0, 1.0}},
        {{ 1.0, -1.0, 1.0 }, {0.0, 0.0}},
        {{-1.0, -1.0, 0.0 }, {1.0, 0.0}},
        {{ 1.0,  1.0, 0.0 }, {0.0, 1.0}},
        {{-1.0, -1.0, 1.0 }, {1.0, 0.0}},
        {{ 1.0, -1.0, 0.0 }, {1.0, 1.0}},
    };

    VkDeviceSize DataSize = sizeof(SPositionUVPointData) * PointData.size();
    m_VertexNum = PointData.size();

    VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
    Vulkan::stageFillBuffer(m_PhysicalDevice, m_Device, PointData.data(), DataSize, m_VertexDataPack.Buffer, m_VertexDataPack.Memory);
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SSkyUniformBufferObjectVert);
    m_VertUniformBufferPacks.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        Vulkan::createBuffer(m_PhysicalDevice, m_Device, VertBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPacks[i].Buffer, m_VertUniformBufferPacks[i].Memory);
    }

    // sampler
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

    // placeholder image
    uint8_t Data = 0;
    auto pTinyImage = std::make_shared<CIOImage>();
    pTinyImage->setSize(1, 1);
    pTinyImage->setData(&Data);
    __createImageFromIOImage(pTinyImage, m_PlaceholderImagePack);
}

void CPipelineSprite::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("Sampler", 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Texture", 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineSprite::MaxSpriteNum), VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
}

void CPipelineSprite::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SSkyUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = nullptr;
        SamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {SamplerInfo} }));

        const size_t NumTexture = m_SpriteImagePackSet.size();
        std::vector<VkDescriptorImageInfo> TexImageInfoSet(CPipelineSprite::MaxSpriteNum);
        for (size_t i = 0; i < CPipelineSprite::MaxSpriteNum; ++i)
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
                TexImageInfoSet[i].imageView = m_SpriteImagePackSet[i].ImageView;
                TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
            }
        }
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageInfoSet }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CPipelineSprite::__createImageFromIOImage(std::shared_ptr<CIOImage> vImage, Vulkan::SImagePack& voImagePack)
{
    size_t TexWidth = vImage->getWidth();
    size_t TexHeight = vImage->getHeight();
    const void* pPixelData = vImage->getData();

    VkDeviceSize DataSize = static_cast<uint64_t>(4) * TexWidth * TexHeight;

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = static_cast<uint32_t>(TexWidth);
    ImageInfo.extent.height = static_cast<uint32_t>(TexHeight);
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
    Vulkan::stageFillImage(m_PhysicalDevice, m_Device, pPixelData, DataSize, ImageInfo, voImagePack.Image, voImagePack.Memory);
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    voImagePack.ImageView = Vulkan::createImageView(m_Device, voImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}
