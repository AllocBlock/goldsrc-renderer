#include "PipelineSprite.h"
#include "Vulkan.h"
#include "Function.h"

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

struct SUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::vec3 EyePosition;
    alignas(16) glm::vec3 EyeDirection;
};

void CPipelineSprite::destroy()
{
    if (m_Device == VK_NULL_HANDLE) return;

    if (m_TextureSampler != VK_NULL_HANDLE)
        vkDestroySampler(m_Device, m_TextureSampler, nullptr);

    for (auto& pImage : m_SpriteImageSet)
        pImage->destroy();
    m_SpriteImageSet.clear();

    m_pPlaceholderImage->destroy();

    m_pVertexDataBuffer->destroy();
    for (auto& pBuffer : m_VertUniformBufferSet)
        pBuffer->destroy();
    m_VertUniformBufferSet.clear();

    IPipeline::destroy();
}

void CPipelineSprite::setSprites(const std::vector<SGoldSrcSprite>& vSpriteImageSet)
{
    _ASSERTE(vSpriteImageSet.size() <= CPipelineSprite::MaxSpriteNum);
    // 为图标创建vkimage
    for (auto& pImage : m_SpriteImageSet)
        pImage->destroy();
    m_SpriteImageSet.resize(vSpriteImageSet.size());
    m_SpriteSequence.resize(vSpriteImageSet.size());
    for (size_t i = 0; i < vSpriteImageSet.size(); ++i)
    {
        m_SpriteImageSet[i] = Function::createImageFromIOImage(m_PhysicalDevice, m_Device, vSpriteImageSet[i].pImage);
        m_SpriteSequence[i].SpriteType = static_cast<uint32_t>(vSpriteImageSet[i].Type);
        m_SpriteSequence[i].Origin = vSpriteImageSet[i].Position;
        m_SpriteSequence[i].Angle = vSpriteImageSet[i].Angle;
        m_SpriteSequence[i].Scale = vSpriteImageSet[i].Scale;
        m_SpriteSequence[i].TexIndex = i;
    }

    __updateDescriptorSet();
}

void CPipelineSprite::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, glm::vec3 vEyeDirection)
{
    SUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = vProj;
    UBOVert.View = vView;
    UBOVert.EyePosition = vEyePos;
    UBOVert.EyeDirection = vEyeDirection;

    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineSprite::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    if (m_pVertexDataBuffer->isValid())
    {
        VkBuffer Buffer = m_pVertexDataBuffer->get();
        const VkDeviceSize Offsets[] = { 0 };
        bind(vCommandBuffer, vImageIndex);
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &Buffer, Offsets);
        for (auto& PushConstant : m_SpriteSequence)
        {
            vkCmdPushConstants(vCommandBuffer, m_PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &PushConstant);
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
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

std::vector<VkPushConstantRange> CPipelineSprite::_getPushConstantRangeSetV()
{
    VkPushConstantRange PushConstantVertInfo = {};
    PushConstantVertInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    PushConstantVertInfo.offset = 0;
    PushConstantVertInfo.size = sizeof(SSpritePushConstant);

    VkPushConstantRange PushConstantFragInfo = {};
    PushConstantFragInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantFragInfo.offset = 0;
    PushConstantFragInfo.size = sizeof(SSpritePushConstant);

    return { PushConstantVertInfo, PushConstantFragInfo };
}

VkPipelineDepthStencilStateCreateInfo CPipelineSprite::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_FALSE;
    DepthStencilInfo.depthWriteEnable = VK_FALSE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

void CPipelineSprite::_getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment)
{
    // result color = source color * source alpha + dst(old) color * (1 - source alpha)
    // result alpha = source alpha
    voBlendAttachment = {};
    voBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    voBlendAttachment.blendEnable = VK_TRUE;
    voBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    voBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    voBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    voBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    voBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    voBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void CPipelineSprite::_initPushConstantV(VkCommandBuffer vCommandBuffer)
{
    SSpritePushConstant Data;
    vkCmdPushConstants(vCommandBuffer, m_PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Data), &Data);
}

void CPipelineSprite::_createResourceV(size_t vImageNum)
{
    // create unit square facing positive x-axis
    // 顺时针
    const std::vector<SPositionUVPointData> PointData =
    {
        {{0.0,  1.0,  1.0 }, {1.0, 1.0}},
        {{0.0,  1.0, -1.0 }, {1.0, 0.0}},
        {{0.0, -1.0, -1.0 }, {0.0, 0.0}},
        {{0.0,  1.0,  1.0 }, {1.0, 1.0}},
        {{0.0, -1.0, -1.0 }, {0.0, 0.0}},
        {{0.0, -1.0,  1.0 }, {0.0, 1.0}},
    };

    VkDeviceSize DataSize = sizeof(SPositionUVPointData) * PointData.size();
    m_VertexNum = PointData.size();
    m_pVertexDataBuffer = make<vk::CBuffer>();
    m_pVertexDataBuffer->create(m_PhysicalDevice, m_Device, DataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertexDataBuffer->stageFill(PointData.data(), DataSize);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUniformBufferObjectVert);
    m_VertUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = make<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_PhysicalDevice, m_Device, VertBufferSize);
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
    uint8_t Data[4] = { 0, 0, 0, 0 };
    auto pTinyImage = make<CIOImage>();
    pTinyImage->setSize(1, 1);
    pTinyImage->setData(Data);
    m_pPlaceholderImage = Function::createImageFromIOImage(m_PhysicalDevice, m_Device, pTinyImage);
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
        VertBufferInfo.buffer = m_VertUniformBufferSet[i]->get();
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = nullptr;
        SamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {SamplerInfo} }));

        const size_t NumTexture = m_SpriteImageSet.size();
        std::vector<VkDescriptorImageInfo> TexImageInfoSet(CPipelineSprite::MaxSpriteNum);
        for (size_t i = 0; i < CPipelineSprite::MaxSpriteNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                if (i == 0) // no texture, use default placeholder texture
                {
                    TexImageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    TexImageInfoSet[i].imageView = m_pPlaceholderImage->get();
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
                TexImageInfoSet[i].imageView = m_SpriteImageSet[i]->get();
                TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
            }
        }
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageInfoSet }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}