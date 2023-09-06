#include "PipelineOutlineEdge.h"
#include "ImageUtils.h"

#include <glm/glm.hpp>

namespace
{
    struct SUBOFrag
    {
        glm::uvec2 ImageExtent;
    };
}

void CPipelineEdge::setInputImage(VkImageView vImageView, size_t vIndex)
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_FragUbufferSet[vIndex]);
    WriteInfo.addWriteImageAndSampler(1, vImageView != VK_NULL_HANDLE ? vImageView : *m_pPlaceholderImage, m_Sampler);
    m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
}

void CPipelineEdge::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UBOFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Input", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineEdge::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("outlineEdgeShader.vert");
    Descriptor.setFragShaderPath("outlineEdgeShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();

    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);
        
    Descriptor.setEnableBlend(true);
    Descriptor.setBlendMethod(EBlendFunction::NORMAL);

    return Descriptor;
}

void CPipelineEdge::_createV()
{
    if (!m_pPlaceholderImage)
    {
        m_pPlaceholderImage = make<vk::CImage>();
        ImageUtils::createPlaceholderImage(*m_pPlaceholderImage, m_pDevice);
    }

    if (!m_Sampler.isValid())
    {
        const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
        VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
            VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, Properties.limits.maxSamplerAnisotropy
        );
        m_Sampler.create(m_pDevice, SamplerInfo);
    }

    m_FragUbufferSet.destroyAndClearAll();
    m_FragUbufferSet.init(m_ImageNum);
    for (size_t i = 0; i < m_ImageNum; ++i)
    {
        m_FragUbufferSet[i]->create(m_pDevice, sizeof(SUBOFrag));
        __updateUniformBuffer(i);
    }

    __initAllDescriptorSet();
}

void CPipelineEdge::_destroyV()
{
    destroyAndClear(m_pPlaceholderImage);
    m_Sampler.destroy();
    m_FragUbufferSet.destroyAndClearAll();
}

void CPipelineEdge::__updateUniformBuffer(uint32_t vImageIndex)
{
    SUBOFrag UBOFrag = {};
    UBOFrag.ImageExtent = glm::uvec2(m_Extent.width, m_Extent.height);
    m_FragUbufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineEdge::__initAllDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_FragUbufferSet[i]);
        WriteInfo.addWriteImageAndSampler(1, *m_pPlaceholderImage, m_Sampler);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}
