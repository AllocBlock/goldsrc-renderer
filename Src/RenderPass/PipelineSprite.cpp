#include "PipelineSprite.h"
#include "ImageUtils.h"
#include "VertexAttributeDescriptor.h"

const size_t CPipelineSprite::MaxSpriteNum = 16; // if need change, you should change this in shader as well

namespace
{
    struct SPointData
    {
        glm::vec3 Pos;
        glm::vec2 TexCoord;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
            return Descriptor.generate();
        }
    };

    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
        alignas(16) glm::vec3 EyePosition;
        alignas(16) glm::vec3 EyeDirection;
    };
}

void CPipelineSprite::setSprites(const std::vector<SGoldSrcSprite>& vSpriteImageSet)
{
    _ASSERTE(vSpriteImageSet.size() <= CPipelineSprite::MaxSpriteNum);

    m_SpriteImageSet.init(vSpriteImageSet.size());
    m_SpriteSequence.resize(vSpriteImageSet.size());
    for (size_t i = 0; i < vSpriteImageSet.size(); ++i)
    {
        ImageUtils::createImageFromIOImage(*m_SpriteImageSet[i], m_pDevice, vSpriteImageSet[i].pImage);
        m_SpriteSequence[i].SpriteType = static_cast<uint32_t>(vSpriteImageSet[i].Type);
        m_SpriteSequence[i].Origin = vSpriteImageSet[i].Position;
        m_SpriteSequence[i].Angle = vSpriteImageSet[i].Angle;
        m_SpriteSequence[i].Scale = vSpriteImageSet[i].Scale;
        m_SpriteSequence[i].TexIndex = static_cast<uint32_t>(i);
    }

    __updateDescriptorSet();
}

void CPipelineSprite::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.EyePosition = vCamera->getPos();
    UBOVert.EyeDirection = vCamera->getFront();

    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineSprite::recordCommand(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex)
{
    if (m_pVertexDataBuffer->isValid())
    {
        bind(vCommandBuffer, vImageIndex);
        vCommandBuffer->bindVertexBuffer(*m_pVertexDataBuffer);
        for (auto& PushConstant : m_SpriteSequence)
        {
            vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
            vCommandBuffer->draw(0, static_cast<uint32_t>(m_VertexNum));
        }
    }
}

void CPipelineSprite::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("Sampler", 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Texture", 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineSprite::MaxSpriteNum), VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineSprite::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("sprShader.vert");
    Descriptor.setFragShaderPath("sprShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.addPushConstant<SSpritePushConstant>(VK_SHADER_STAGE_VERTEX_BIT);
    Descriptor.addPushConstant<SSpritePushConstant>(VK_SHADER_STAGE_FRAGMENT_BIT);

    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);
    
    Descriptor.setEnableBlend(true);
    Descriptor.setBlendMethod(EBlendFunction::NORMAL);

    return Descriptor;
}

void CPipelineSprite::_initPushConstantV(CCommandBuffer::Ptr vCommandBuffer)
{
    SSpritePushConstant Data;
    vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Data);
}

void CPipelineSprite::_createV()
{
    // create unit square facing positive x-axis
    const std::vector<SPointData> PointData =
    {
        {{0.0,  1.0,  1.0 }, {1.0, 1.0}},
        {{0.0,  1.0, -1.0 }, {1.0, 0.0}},
        {{0.0, -1.0, -1.0 }, {0.0, 0.0}},
        {{0.0,  1.0,  1.0 }, {1.0, 1.0}},
        {{0.0, -1.0, -1.0 }, {0.0, 0.0}},
        {{0.0, -1.0,  1.0 }, {0.0, 1.0}},
    };

    VkDeviceSize DataSize = sizeof(SPointData) * PointData.size();
    m_VertexNum = PointData.size();
    m_pVertexDataBuffer = make<vk::CBuffer>();
    m_pVertexDataBuffer->create(m_pDevice, DataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertexDataBuffer->stageFill(PointData.data(), DataSize);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_VertUniformBufferSet.init(m_ImageNum);

    for (size_t i = 0; i < m_ImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
    }

    // sampler
    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    // placeholder image
    ImageUtils::createPlaceholderImage(m_PlaceholderImage, m_pDevice);
}

void CPipelineSprite::_destroyV()
{
    m_Sampler.destroy();
    m_SpriteImageSet.destroyAndClearAll();
    m_PlaceholderImage.destroy();

    destroyAndClear(m_pVertexDataBuffer);
    m_VertUniformBufferSet.destroyAndClearAll();
}

void CPipelineSprite::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteSampler(1, m_Sampler);

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
                TexImageViewSet[i] = *m_SpriteImageSet[i];
        }

        WriteInfo.addWriteImagesAndSampler(2, TexImageViewSet);

        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}
