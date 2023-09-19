#include "PipelineSSAO.h"
#include "FullScreenPointData.h"
#include "ImageUtils.h"
#include "InterfaceGui.h"

namespace
{
    struct SUBOFrag
    {
        glm::mat4 VP = glm::mat4(1.0f);
        glm::mat4 InversedVP = glm::mat4(1.0f);
        float Strength = 0.1;
        int SampleNum = 16;
        float SampleRadius = 1.0f;
        float Time = 0.0f;
    };
}

void CPipelineSSAO::setDepthImage(VkImageView vImageView, size_t vIndex)
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteImageAndSampler(1, vImageView != VK_NULL_HANDLE ? vImageView : m_PlaceholderImage, m_Sampler);
    m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
}

void CPipelineSSAO::updateUniformBuffer(uint32_t vImageIndex, CCamera::Ptr vCamera)
{
    SUBOFrag UBOFrag = SUBOFrag();
    UBOFrag.VP = vCamera->getViewProjMat();
    UBOFrag.InversedVP = glm::inverse(vCamera->getViewProjMat());
    UBOFrag.Strength = m_Strength;
    UBOFrag.SampleNum = m_SampleNum;
    UBOFrag.SampleRadius = m_SampleRadius;
    UBOFrag.Time = m_Time;
    m_FragUBSet[vImageIndex]->update(&UBOFrag);
    m_Time += 0.1f;
}

void CPipelineSSAO::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Input", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineSSAO::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("fullScreen.vert");
    Descriptor.setFragShaderPath("ssao.frag");

    Descriptor.setVertexInputInfo<SFullScreenPointData>();
    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);
    Descriptor.setEnableBlend(true);
    Descriptor.setBlendMethod(EBlendFunction::MULTIPLY);

    return Descriptor;
}

void CPipelineSSAO::_createV()
{
    VkDeviceSize FragBufferSize = sizeof(SUBOFrag);
    m_FragUBSet.init(m_ImageNum);

    for (size_t i = 0; i < m_ImageNum; ++i)
    {
        m_FragUBSet[i]->create(m_pDevice, FragBufferSize);
    }

    if (!m_PlaceholderImage.isValid())
    {
        ImageUtils::createPlaceholderImage(m_PlaceholderImage, m_pDevice);
    }

    if (!m_Sampler.isValid())
    {
        const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
        VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
            VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, Properties.limits.maxSamplerAnisotropy
        );
        m_Sampler.create(m_pDevice, SamplerInfo);
    }

    __initAllDescriptorSet();
}

void CPipelineSSAO::_destroyV()
{
    m_FragUBSet.destroyAndClearAll();
    m_PlaceholderImage.destroy();
    m_Sampler.destroy();
}

void CPipelineSSAO::_renderUIV()
{
    if (UI::collapse("SSAO"))
    {
        UI::drag("Strength", m_Strength, 0.1f, 0.1f, 3.0f);
        UI::drag("Sample Num", m_SampleNum, 2, 4, 128);
        UI::drag("Radius", m_SampleRadius, 0.001f, 0.01f, 0.5f);
    }
}

void CPipelineSSAO::__initAllDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_FragUBSet[i]);
        WriteInfo.addWriteImageAndSampler(1, m_PlaceholderImage, m_Sampler);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}
