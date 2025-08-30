#include "PipelineText.h"
#include "ImageUtils.h"

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

void CPipelineText::updateUniformBuffer(CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.EyePosition = vCamera->getPos();
    UBOVert.EyeDirection = vCamera->getFront();

    m_pVertUniformBuffer->update(&UBOVert);
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

bool CPipelineText::doesNeedRerecord()
{
    return m_NeedRerecord;
}

// return false if no command is recorded
bool CPipelineText::recordCommand(CCommandBuffer::Ptr vCommandBuffer)
{
    _ASSERTE(doesNeedRerecord());

    for (size_t i = 0; i < m_TextRendererSet.size(); ++i)
    {
        if (m_NeedUpdateVertBufferSet[i])
        {
            m_pDevice->waitUntilIdle();
            if (m_VertBufferSet[i]) m_VertBufferSet[i]->destroy();
            m_VertBufferSet[i] = m_TextRendererSet[i]->generateVertexBuffer(m_pDevice);
            m_NeedUpdateVertBufferSet[i] = false;
        }

        auto pVertBuffer = m_VertBufferSet[i];

        if (!pVertBuffer) continue;
        bind(vCommandBuffer);
        vCommandBuffer->bindVertexBuffer(*pVertBuffer);
        
        SPushConstant Constant;
        Constant.Position = m_TextRendererSet[i]->getWorldPosition();
        Constant.Scale = m_TextRendererSet[i]->getTransform()->getAbsoluteScale();
        vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Constant);
        vCommandBuffer->draw(0, pVertBuffer->getVertexNum());
    }
    m_NeedRerecord = false;

    return !m_TextRendererSet.empty();
}

void CPipelineText::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
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

void CPipelineText::_createV()
{
    // uniform buffer
    m_pVertUniformBuffer = make<vk::CUniformBuffer>();
    m_pVertUniformBuffer->create(m_pDevice, sizeof(SUBOVert));

    // sampler
    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    // default font image
    ImageUtils::createImageFromIOImage(m_FontImage, m_pDevice, CFont::getDefaultFont()->getImage());

    __updateDescriptorSet();

    m_NeedRerecord = true;
}

void CPipelineText::_destroyV()
{
    clearTextComponent();

    m_Sampler.destroy();
    m_FontImage.destroy();
    destroyAndClear(m_pVertUniformBuffer);
}

void CPipelineText::__updateDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_pVertUniformBuffer);
    WriteInfo.addWriteImageAndSampler(1, m_FontImage, m_Sampler);
    m_ShaderResourceDescriptor.update(WriteInfo);
}

inline void CPipelineText::__markNeedRerecord()
{
    m_NeedRerecord = true;
}
