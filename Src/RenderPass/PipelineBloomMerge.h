#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "FullScreenPointData.h"
#include "ImageUtils.h"
#include "InterfaceUI.h"

namespace
{
    struct SUBOFrag
    {
        alignas(16) float BloomFactor;
    };
}

class CPipelineBloomMerge : public IPipeline
{

public:
    void setInputImage(VkImageView vImageView, size_t vIndex)
    {
        __updateInputImage(vImageView, vIndex);
    }

protected:
    virtual void _initShaderResourceDescriptorV() override
    {
        _ASSERTE(m_pDevice != VK_NULL_HANDLE);
        m_ShaderResourceDescriptor.clear();
        m_ShaderResourceDescriptor.add("UBOFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_ShaderResourceDescriptor.add("Input", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_ShaderResourceDescriptor.createLayout(m_pDevice);
    }

    virtual CPipelineDescriptor _getPipelineDescriptionV() override
    {
        CPipelineDescriptor Descriptor;

        Descriptor.setVertShaderPath("fullScreen.vert");
        Descriptor.setFragShaderPath("bloomBlur.frag");

        Descriptor.setVertexInputInfo<SFullScreenPointData>();

        return Descriptor;
    }

    virtual void _createV() override
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
        
        m_FragUbufferSet.init(vImageNum);
        for (size_t i = 0; i < vImageNum; ++i)
        {
            m_FragUbufferSet[i]->create(m_pDevice, sizeof(SUBOFrag));
            __updateUniformBuffer(i);
        }

        __initAllDescriptorSet();
    }

    virtual void _destroyV() override
    {
        destroyAndClear(m_pPlaceholderImage);
        m_Sampler.destroy();
        m_FragUbufferSet.destroyAndClearAll();
    }

    virtual void _renderUIV()
    {
        if (UI::collapse("Pipeline Bloom Blur"))
        {
            UI::drag(u8"Ä£ºý·¶Î§", m_FilterSize, 0.1f, 1.0f, 15.0f);
        }
    }

private:
    void __updateInputImages(std::vector<VkImageView> vImageViewSet)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteInputAttachment(1, vImageViewSet);
        m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
    }

    void __updateUniformBuffer(uint32_t vImageIndex)
    {
        SUBOFrag UBOFrag = {};
        //UBOFrag.FilterSize = m_FilterSize;
        UBOFrag.ImageExtent = m_Extent;
        m_FragUbufferSet[vImageIndex]->update(&UBOFrag);
    }

    void __initAllDescriptorSet()
    {
        for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
        {
            CDescriptorWriteInfo WriteInfo;
            WriteInfo.addWriteBuffer(0, *m_FragUbufferSet[i]);
            WriteInfo.addWriteInputAttachment(1, *m_pPlaceholderImage, m_Sampler);
            m_ShaderResourceDescriptor.update(i, WriteInfo);
        }
    }

    vk::CPointerSet<vk::CUniformBuffer> m_FragUbufferSet;

    float m_FilterSize = 5.0f;
};
