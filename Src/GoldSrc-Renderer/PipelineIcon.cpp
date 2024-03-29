#include "PipelineIcon.h"
#include "ImageUtils.h"
#include "VertexAttributeDescriptor.h"

namespace
{
    struct SPointData
    {
        glm::vec2 Pos;
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

    struct SPushConstant
    {
        uint32_t TexIndex = 0;
        uint32_t BlendType = 0;
        float Scale = 1.0f;
        alignas(16) glm::vec3 Position = glm::vec3(0.0f);
    };
}

const size_t CPipelineIcon::MaxIconNum = 128;

void CPipelineIcon::addIcon(EIcon vIcon, glm::vec3 vPosition, float vScale)
{
    m_IconInfoSet.push_back({ vIcon, vPosition, vScale });
}

void CPipelineIcon::clear()
{
    m_IconInfoSet.clear();
}

void CPipelineIcon::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.EyePosition = vCamera->getPos();
    UBOVert.EyeDirection = vCamera->getFront();

    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineIcon::recordCommand(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex)
{
    if (m_pVertexDataBuffer->isValid())
    {
        bind(vCommandBuffer, vImageIndex);
        vCommandBuffer->bindVertexBuffer(*m_pVertexDataBuffer);
        
        auto pIconManager = CIconManager::getInstance();

        SPushConstant Constant;
        for (const auto& IconInfo : m_IconInfoSet)
        {
            Constant.BlendType = uint32_t(pIconManager->getRenderType(IconInfo.Icon));
            Constant.TexIndex = m_IconIndexMap.at(IconInfo.Icon);
            Constant.Position = IconInfo.Position;
            Constant.Scale = IconInfo.Scale;
            vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Constant);
            vCommandBuffer->draw(0, static_cast<uint32_t>(m_VertexNum));
        }
    }
}

void CPipelineIcon::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("Sampler", 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Texture", 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineIcon::MaxIconNum), VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineIcon::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("iconShader.vert");
    Descriptor.setFragShaderPath("iconShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.addPushConstant<SPushConstant>(VK_SHADER_STAGE_VERTEX_BIT);
    Descriptor.addPushConstant<SPushConstant>(VK_SHADER_STAGE_FRAGMENT_BIT);

    Descriptor.setEnableDepthTest(true);
    Descriptor.setEnableDepthWrite(true);
    // TODO: turn on depth write, opaque and indexed transparent work fine, but alpha blend is strange
    // turn off, opaque and indexed transparent is bad, but alpha blend works better

    /*Descriptor.setEnableBlend(true);
    Descriptor.setBlendMethod(EBlendFunction::NORMAL);*/

    return Descriptor;
}

void CPipelineIcon::_initPushConstantV(CCommandBuffer::Ptr vCommandBuffer)
{
    SPushConstant Data;
    vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Data);
}

void CPipelineIcon::_createV()
{
    // create unit square facing positive x-axis
    const std::vector<SPointData> PointData =
    {
        {{-1.0,  1.0 }, {0.0, 0.0}},
        {{-1.0, -1.0 }, {0.0, 1.0}},
        {{ 1.0, -1.0 }, {1.0, 1.0}},
        {{-1.0,  1.0 }, {0.0, 0.0}},
        {{ 1.0, -1.0 }, {1.0, 1.0}},
        {{ 1.0,  1.0 }, {1.0, 0.0}},
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

    // load icon images
    __createIconResources();
}

void CPipelineIcon::_destroyV()
{
    m_Sampler.destroy();
    m_IconImageSet.destroyAndClearAll();
    m_PlaceholderImage.destroy();

    destroyAndClear(m_pVertexDataBuffer);
    m_VertUniformBufferSet.destroyAndClearAll();
}

void CPipelineIcon::__createIconResources()

{
    auto pIconManager = CIconManager::getInstance();

    m_IconNum = uint32_t(EIcon::MAX_NUM);
    m_IconImageSet.init(m_IconNum);
    for (uint32_t i = 0; i < m_IconNum; ++i)
    {
        EIcon Icon = EIcon(i);
        auto pImage = pIconManager->getImage(Icon);
        ImageUtils::createImageFromIOImage(*m_IconImageSet[i], m_pDevice, pImage);
        m_IconIndexMap[Icon] = i;
    }
    __updateDescriptorSet();
}

void CPipelineIcon::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteSampler(1, m_Sampler);

        const size_t NumTexture = m_IconImageSet.size();
        std::vector<VkImageView> TexImageViewSet(CPipelineIcon::MaxIconNum);
        for (size_t i = 0; i < CPipelineIcon::MaxIconNum; ++i)
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
                TexImageViewSet[i] = *m_IconImageSet[i];
        }

        WriteInfo.addWriteImagesAndSampler(2, TexImageViewSet);

        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}
