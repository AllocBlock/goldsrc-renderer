#include "PipelineText.h"
#include "Function.h"
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

void CPipelineText::drawTextActor(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex, CActor::Ptr vActor)
{
    CComponentTextRenderer::CPtr pTextRenderer = vActor->getTransform()->findComponent<CComponentTextRenderer>();
    if (!pTextRenderer) return;
    if (m_TextCompVertBufferMap.find(pTextRenderer) == m_TextCompVertBufferMap.end())
    {
        m_TextCompVertBufferMap[pTextRenderer] = pTextRenderer->generateVertexBuffer(m_pDevice); // what if text is updated
    }

    _ASSERTE(pTextRenderer->getFont() == CFont::getDefaultFont()); // only static default font is support for now

    auto pVertBuffer = m_TextCompVertBufferMap.at(pTextRenderer);
    bind(vCommandBuffer, vImageIndex);
    vCommandBuffer->bindVertexBuffer(*pVertBuffer);

    SPushConstant Constant;
    Constant.Position = pTextRenderer->getWorldPosition();
    Constant.Scale = vActor->getTransform()->getAbsoluteScale();
    vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Constant);
    vCommandBuffer->draw(0, pVertBuffer->getVertexNum());
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

    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);

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
    Function::createImageFromIOImage(m_FontImage, m_pDevice, CFont::getDefaultFont()->getImage());

    __updateDescriptorSet();
}

void CPipelineText::_destroyV()
{
    m_Sampler.destroy();
    m_FontImage.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();

    for (const auto& Pair : m_TextCompVertBufferMap)
    {
        Pair.second->destroy();
    }
    m_TextCompVertBufferMap.clear();
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
