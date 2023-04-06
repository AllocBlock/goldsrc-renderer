#include "PipelineText.h"
#include "ImageUtils.h"
#include "VertexAttributeDescriptor.h"

namespace
{
    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
        alignas(16) glm::vec3 EyePosition;
        alignas(16) glm::vec3 EyeDirection;
    };

    struct SPushConstant
    {
        alignas(16) glm::vec3 Scale = glm::vec3(1.0f);
        alignas(16) glm::vec3 Position = glm::vec3(0.0f);
    };
}

void CPipelineText::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.EyePosition = vCamera->getPos();
    UBOVert.EyeDirection = vCamera->getFront();

    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineText::addTextComponent(CComponentTextRenderer::Ptr vTextRenderer)
{
    _ASSERTE(vTextRenderer);
    _ASSERTE(vTextRenderer->getTransform());
    _ASSERTE(vTextRenderer->getFont() == CFont::getDefaultFont()); // only static default font is support for now

    m_TextRendererSet.emplace_back(vTextRenderer);
    m_VertBufferSet.emplace_back(nullptr);
    m_NeedUpdateVertBufferSet.emplace_back(true);

    size_t Index = m_TextRendererSet.size() - 1;
    vTextRenderer->hookTextMeshUpdate([this, Index]() { m_NeedUpdateVertBufferSet[Index] = true; __markNeedRerecord(); });
    vTextRenderer->hookShaderParamUpdate([this, Index]() { __markNeedRerecord(); });

}

void CPipelineText::clearTextComponent()
{
    m_TextRendererSet.clear();
    for (const auto& pBuffer : m_VertBufferSet)
    {
        pBuffer->destroy();
    }
    m_VertBufferSet.clear();
    m_NeedUpdateVertBufferSet.clear();
    __markNeedRerecord();
}

bool CPipelineText::doesNeedRerecord(size_t vImageIndex)
{
    return m_NeedRerecordSet[vImageIndex];
}

// return false if no command is recorded
bool CPipelineText::recordCommand(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex)
{
    _ASSERTE(doesNeedRerecord(vImageIndex));

    for (size_t i = 0; i < m_TextRendererSet.size(); ++i)
    {
        if (m_NeedUpdateVertBufferSet[i])
        {
            m_VertBufferSet[i] = m_TextRendererSet[i]->generateVertexBuffer(m_pDevice);
            m_NeedUpdateVertBufferSet[i] = false;
        }

        auto pVertBuffer = m_VertBufferSet[i];
        bind(vCommandBuffer, vImageIndex);
        vCommandBuffer->bindVertexBuffer(*pVertBuffer);


        SPushConstant Constant;
        Constant.Position = m_TextRendererSet[i]->getWorldPosition();
        Constant.Scale = m_TextRendererSet[i]->getTransform()->getAbsoluteScale();
        vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Constant);
        vCommandBuffer->draw(0, pVertBuffer->getVertexNum());
    }
    m_NeedRerecordSet[vImageIndex] = false;

    return !m_TextRendererSet.empty();
}

void CPipelineText::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("SampledTexture", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineText::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("textShader.vert");
    Descriptor.setFragShaderPath("textShader.frag");

    Descriptor.setVertexInputInfo<CComponentTextRenderer::SPointData>();
    Descriptor.addPushConstant<SPushConstant>(VK_SHADER_STAGE_VERTEX_BIT);
    Descriptor.addPushConstant<SPushConstant>(VK_SHADER_STAGE_FRAGMENT_BIT);

    Descriptor.setEnableDepthTest(true);
    Descriptor.setEnableDepthWrite(true);

    return Descriptor;
}

void CPipelineText::_initPushConstantV(CCommandBuffer::Ptr vCommandBuffer)
{
    SPushConstant Data;
    vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Data);
}

void CPipelineText::_createResourceV(size_t vImageNum)
{
    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_VertUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
    }

    // sampler
    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    // default font image
    ImageUtils::createImageFromIOImage(m_FontImage, m_pDevice, CFont::getDefaultFont()->getImage());

    __updateDescriptorSet();

    m_NeedRerecordSet.clear();
    m_NeedRerecordSet.resize(vImageNum, true);
}

void CPipelineText::_destroyV()
{
    clearTextComponent();
    m_NeedRerecordSet.clear();

    m_Sampler.destroy();
    m_FontImage.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();
}

void CPipelineText::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteImageAndSampler(1, m_FontImage,m_Sampler);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}
