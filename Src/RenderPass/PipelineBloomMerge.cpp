#include "PipelineBloomMerge.h"
#include "FullScreenPointData.h"
#include "ImageUtils.h"

namespace
{
    struct SUBOFrag
    {
        float BloomFactor;
    };
}

void CPipelineBloomMerge::setInputImage(VkImageView vBase, VkImageView vBlur, size_t vIndex)
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteImageAndSampler(1, vBase, m_Sampler);
    WriteInfo.addWriteImageAndSampler(2, vBlur, m_Sampler);
    m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
}

void CPipelineBloomMerge::updateUniformBuffer(uint32_t vImageIndex, float vBloomFactor)
{
    SUBOFrag UBOFrag = {};
    UBOFrag.BloomFactor = vBloomFactor;
    m_FragUbufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineBloomMerge::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UBOFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("BaseImage", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("BlurImage", 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineBloomMerge::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("fullScreen.vert");
    Descriptor.setFragShaderPath("bloomMerge.frag");

    Descriptor.setVertexInputInfo<SFullScreenPointData>();

    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);

    return Descriptor;
}

void CPipelineBloomMerge::_createV()
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
        
    m_FragUbufferSet.init(m_ImageNum);
    for (size_t i = 0; i < m_ImageNum; ++i)
    {
        m_FragUbufferSet[i]->create(m_pDevice, sizeof(SUBOFrag));
        updateUniformBuffer(i, 0.5f);
    }

    __initAllDescriptorSet();
}

void CPipelineBloomMerge::_destroyV()
{
    m_PlaceholderImage.destroy();
    m_Sampler.destroy();
    m_FragUbufferSet.destroyAndClearAll();
}

void CPipelineBloomMerge::_renderUIV()
{
}

void CPipelineBloomMerge::__initAllDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_FragUbufferSet[i]);
        WriteInfo.addWriteImageAndSampler(1, m_PlaceholderImage, m_Sampler);
        WriteInfo.addWriteImageAndSampler(2, m_PlaceholderImage, m_Sampler);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}
