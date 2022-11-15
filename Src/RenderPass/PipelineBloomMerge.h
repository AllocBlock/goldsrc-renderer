#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "FullScreenPointData.h"
#include "Function.h"
#include "InterfaceUI.h"
#include "RenderPassPort.h"

namespace
{
    struct SUBOFrag
    {
        alignas(16) float BloomFactor;
    };
}

class CPipelineBlendMerge : public IPipeline
{
public:
    void setInputPort(CPort::Ptr vInputPort, CPort::Ptr vBlurPort)
    {
        vInputPort->hookImageUpdate([this, vInputPort]()
            {
                if (!vInputPort->isImageReadyV()) return;
                for (size_t i = 0; i < m_ImageNum; ++i)
                {
                    __updateInputImage(vInputPort->getImageV(i), i);
                }
            });
        
        vBlurPort->hookImageUpdate([this, vBlurPort]()
            {
                if (!vBlurPort->isImageReadyV()) return;                for (size_t i = 0; i < m_ImageNum; ++i)
                {
                    __updateInputBlurImage(vBlurPort->getImageV(i), i);
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
        m_ShaderResourceDescriptor.add("Blur", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_ShaderResourceDescriptor.createLayout(m_pDevice);
    }

    virtual CPipelineDescriptor _getPipelineDescriptionV() override
    {
        CPipelineDescriptor Descriptor;

        Descriptor.setVertShaderPath("shaders/fullScreen.vert");
        Descriptor.setFragShaderPath("shaders/bloomMerge.frag");

        Descriptor.setVertexInputInfo<SFullScreenPointData>();

        return Descriptor;
    }

    virtual void _createResourceV(size_t vImageNum) override
    {
        if (!m_pPlaceholderImage)
        {
            m_pPlaceholderImage = make<vk::CImage>();
            Function::createPlaceholderImage(*m_pPlaceholderImage, m_pDevice);
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
        if (UI::collapse("Pipeline Bloom Merge"))
        {
            UI::drag(u8"»ìºÏÏµÊý", m_BloomFactor, 0.01f, 0.0f, 1.0f);
        }
    }

private:
    void __updateInputImage(VkImageView vImageView, size_t vIndex)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteImageAndSampler(1, vImageView, m_Sampler);
        m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
    }

    void __updateInputBlurImage(VkImageView vImageView, size_t vIndex)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteImageAndSampler(2, vImageView, m_Sampler);
        m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
    }

    void __updateUniformBuffer(uint32_t vImageIndex)
    {
        SUBOFrag UBOFrag = {};
        UBOFrag.BloomFactor = m_BloomFactor;
        m_FragUbufferSet[vImageIndex]->update(&UBOFrag);
    }

    void __initAllDescriptorSet()
    {
        for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
        {
            CDescriptorWriteInfo WriteInfo;
            WriteInfo.addWriteBuffer(0, *m_FragUbufferSet[i]);
            WriteInfo.addWriteImageAndSampler(1, *m_pPlaceholderImage, m_Sampler);
            WriteInfo.addWriteImageAndSampler(2, *m_pPlaceholderImage, m_Sampler);
            m_ShaderResourceDescriptor.update(i, WriteInfo);
        }
    }

    vk::CPointerSet<vk::CUniformBuffer> m_FragUbufferSet;
    vk::CImage::Ptr m_pPlaceholderImage = nullptr;
    vk::CSampler m_Sampler;

    float m_BloomFactor = 0.3f;
};
