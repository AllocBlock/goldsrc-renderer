#include "PipelineSprite.h"
#include "Vulkan.h"
#include "Function.h"
#include "VertexAttributeDescriptor.h"

#include <glm/ext/matrix_transform.hpp>

const size_t CPipelineSprite::MaxSpriteNum = 2048;

struct SPositionUVPointData
{
    glm::vec3 Pos;
    glm::vec2 TexCoord;

    using PointData_t = SPositionUVPointData;
    _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
        Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
        return Descriptor.generate();
    }
};

struct SUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::vec3 EyePosition;
    alignas(16) glm::vec3 EyeDirection;
};

void CPipelineSprite::setSprites(const std::vector<SGoldSrcSprite>& vSpriteImageSet)
{
    _ASSERTE(vSpriteImageSet.size() <= CPipelineSprite::MaxSpriteNum);
    // 为图标创建vkimage
    m_SpriteImageSet.destroyAndClearAll();
    m_SpriteImageSet.init(vSpriteImageSet.size());

    m_SpriteSequence.resize(vSpriteImageSet.size());
    for (size_t i = 0; i < vSpriteImageSet.size(); ++i)
    {
        Function::createImageFromIOImage(m_SpriteImageSet[i], m_pDevice, vSpriteImageSet[i].pImage);
        m_SpriteSequence[i].SpriteType = static_cast<uint32_t>(vSpriteImageSet[i].Type);
        m_SpriteSequence[i].Origin = vSpriteImageSet[i].Position;
        m_SpriteSequence[i].Angle = vSpriteImageSet[i].Angle;
        m_SpriteSequence[i].Scale = vSpriteImageSet[i].Scale;
        m_SpriteSequence[i].TexIndex = i;
    }

    __updateDescriptorSet();
}

void CPipelineSprite::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.EyePosition = vCamera->getPos();
    UBOVert.EyeDirection = vCamera->getFront();
    m_VertUniformBufferSet[vImageIndex].update(&UBOVert);
}

void CPipelineSprite::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    if (m_VertexBuffer.isValid())
    {
        VkBuffer Buffer = m_VertexBuffer;
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

    m_VertexBuffer.create(m_pDevice, DataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.stageFill(PointData.data(), DataSize);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUniformBufferObjectVert);
    m_VertUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i].create(m_pDevice, VertBufferSize);
    }
    
    // sampler
    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);
    
    // placeholder image
    Function::createPlaceholderImage(m_PlaceholderImage, m_pDevice);
}

void CPipelineSprite::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("Sampler", 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Texture", 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineSprite::MaxSpriteNum), VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineSprite::_destroyV()
{
    if (m_pDevice == VK_NULL_HANDLE) return;

    m_Sampler.destroy();
    m_SpriteImageSet.destroyAndClearAll();
    m_PlaceholderImage.destroy();
    m_VertexBuffer.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();
}

void CPipelineSprite::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_VertUniformBufferSet[i]);
        WriteInfo.addWriteSampler(1, m_Sampler.get());

        const size_t NumTexture = m_SpriteImageSet.size();
        std::vector<VkImageView> TexImageViewSet(CPipelineSprite::MaxSpriteNum);
        for (size_t i = 0; i < CPipelineSprite::MaxSpriteNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                if (i == 0) // no texture, use default placeholder texture
                    TexImageViewSet[i] = m_PlaceholderImage;
                else
                    TexImageViewSet[i] = TexImageViewSet[0];
            }
            else
                TexImageViewSet[i] = m_SpriteImageSet[i];
        }
        WriteInfo.addWriteImagesAndSampler(2, TexImageViewSet);

        m_Descriptor.update(i, WriteInfo);
    }
}