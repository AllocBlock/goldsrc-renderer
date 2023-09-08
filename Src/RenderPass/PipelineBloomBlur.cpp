#include "PipelineBloomBlur.h"

namespace
{
    struct SUBOFrag
    {
        alignas(16) glm::uvec2 ImageExtent;
    };
}

void CPipelineBloomBlur::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UBOFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Input", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineBloomBlur::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("fullScreen.vert");
    Descriptor.setFragShaderPath("bloomBlur.frag");

    Descriptor.setVertexInputInfo<SFullScreenPointData>();

    return Descriptor;
}

void CPipelineBloomBlur::_createV()
{
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
        
    m_FragUniformBufferSet.init(m_ImageNum);
    for (size_t i = 0; i < m_ImageNum; ++i)
    {
        m_FragUniformBufferSet[i]->create(m_pDevice, sizeof(SUBOFrag));
        __updateUniformBuffer(i);
    }

    __initAllDescriptorSet();
}

void CPipelineBloomBlur::_destroyV()
{
    m_PlaceholderImage.destroy();
    m_Sampler.destroy();
    m_FragUniformBufferSet.destroyAndClearAll();
}

void CPipelineBloomBlur::_renderUIV()
{
    if (UI::collapse("Pipeline Bloom Blur"))
    {
        UI::drag(u8"Ä£ºý·¶Î§", m_FilterSize, 0.1f, 1.0f, 15.0f);
    }
}

void CPipelineBloomBlur::__updateInputImage(VkImageView vImageView, size_t vIndex)
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteImageAndSampler(1, vImageView != VK_NULL_HANDLE ? vImageView : m_PlaceholderImage, m_Sampler);
    m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
}

void CPipelineBloomBlur::__updateUniformBuffer(uint32_t vImageIndex)
{
    SUBOFrag UBOFrag = {};
    //UBOFrag.FilterSize = m_FilterSize;
    UBOFrag.ImageExtent = glm::uvec2(m_Extent.width, m_Extent.height);
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineBloomBlur::__initAllDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_FragUniformBufferSet[i]);
        WriteInfo.addWriteImageAndSampler(1, m_PlaceholderImage, m_Sampler);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}
