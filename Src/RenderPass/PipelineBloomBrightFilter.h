#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "FullScreenPointData.h"
#include "ImageUtils.h"
#include "InterfaceUI.h"
#include "RenderPassPort.h"

namespace
{
    struct SUBOFrag
    {
        alignas(16) float Threshold;
    };
}

class CPipelineBlendBrightFilter : public IPipeline
{
public:
    void setInputPort(CPort::Ptr vPort)
    {
        vPort->hookImageUpdate([this, vPort]()
        {
            if (!vPort->isImageReadyV()) return;
            for (size_t i = 0; i < m_ImageNum; ++i)
            {
                __updateInputImage(vPort->getImageV(i), i);
            }
        });
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
        Descriptor.setFragShaderPath("bloomBrightFilter.frag");

        Descriptor.setVertexInputInfo<SFullScreenPointData>();

        return Descriptor;
    }

    virtual void _createResourceV(size_t vImageNum) override
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
        if (UI::collapse("Pipeline Bloom Bright Filter"))
        {
            UI::drag(u8"ÁÁ¶ÈãÐÖµ", m_Threshold, 0.01f, 0.0f, 1.0f);
        }
    }

private:
    void __updateInputImage(VkImageView vImageView, size_t vIndex)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteImageAndSampler(1, vImageView != VK_NULL_HANDLE ? vImageView : *m_pPlaceholderImage, m_Sampler);
        m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
    }

    void __updateUniformBuffer(uint32_t vImageIndex)
    {
        SUBOFrag UBOFrag = {};
        UBOFrag.Threshold = m_Threshold;
        m_FragUbufferSet[vImageIndex]->update(&UBOFrag);
    }

    void __initAllDescriptorSet()
    {
        for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
        {
            CDescriptorWriteInfo WriteInfo;
            WriteInfo.addWriteBuffer(0, *m_FragUbufferSet[i]);
            WriteInfo.addWriteImageAndSampler(1, *m_pPlaceholderImage, m_Sampler);
            m_ShaderResourceDescriptor.update(i, WriteInfo);
        }
    }

    vk::CPointerSet<vk::CUniformBuffer> m_FragUbufferSet;
    vk::CImage::Ptr m_pPlaceholderImage = nullptr;
    vk::CSampler m_Sampler;

    float m_Threshold = 0.3;
};
