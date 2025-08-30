#include "PipelineBlit.h"
#include "FullScreenPointData.h"
#include "ImageUtils.h"

void CPipelineBlit::setInputImage(VkImageView vImageView)
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteImageAndSampler(0, vImageView != VK_NULL_HANDLE ? vImageView : m_PlaceholderImage, m_Sampler);
    m_ShaderResourceDescriptor.update(WriteInfo);
}

void CPipelineBlit::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("Input", 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineBlit::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("fullScreen.vert");
    Descriptor.setFragShaderPath("blit.frag");

    Descriptor.setVertexInputInfo<SFullScreenPointData>();
    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);

    return Descriptor;
}

void CPipelineBlit::_createV()
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

    __initAllDescriptorSet();
}

void CPipelineBlit::_destroyV()
{
    m_PlaceholderImage.destroy();
    m_Sampler.destroy();
}

void CPipelineBlit::_renderUIV()
{
}

void CPipelineBlit::__initAllDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteImageAndSampler(0, m_PlaceholderImage, m_Sampler);
    m_ShaderResourceDescriptor.update(WriteInfo);
}
