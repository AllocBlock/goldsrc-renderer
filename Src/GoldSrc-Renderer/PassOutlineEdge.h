#pragma once
#include "Vulkan.h"
#include "Common.h"
#include "FrameBuffer.h"
#include "Scene.h"
#include "ShaderResourceDescriptor.h"
#include "Image.h"
#include "Buffer.h"
#include "Sampler.h"
#include "IPipeline.h"
#include "UniformBuffer.h"
#include "VertexAttributeDescriptor.h"
#include "IRenderPass.h"
#include "Function.h"

#include <glm/glm.hpp>

class CPipelineEdge : public IPipeline
{
public:
    struct SPointData
    {
        glm::vec2 Pos;
        glm::vec2 TexCoord;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
            return Descriptor.generate();
        }
    };

    struct SUBOFrag
    {
        glm::uvec2 ImageExtent;
    };

    void setInputImage(VkImageView vImageView, size_t vIndex)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_FragUbufferSet[vIndex]);
        WriteInfo.addWriteImageAndSampler(1, vImageView != VK_NULL_HANDLE ? vImageView : *m_pPlaceholderImage, m_Sampler);
        m_ShaderResourceDescriptor.update(vIndex, WriteInfo);
    }

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/outlineEdgeShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/outlineEdgeShaderFrag.spv"; }

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

        m_FragUbufferSet.destroyAndClearAll();
        m_FragUbufferSet.init(vImageNum);
        for (size_t i = 0; i < vImageNum; ++i)
        {
            m_FragUbufferSet[i] = make<vk::CUniformBuffer>();
            m_FragUbufferSet[i]->create(m_pDevice, sizeof(SUBOFrag));
            __updateUniformBuffer(i);
        }

        __initAllDescriptorSet();
    }

    virtual void _initShaderResourceDescriptorV() override
    {
        _ASSERTE(m_pDevice != VK_NULL_HANDLE);
        m_ShaderResourceDescriptor.clear();
        m_ShaderResourceDescriptor.add("UBOFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_ShaderResourceDescriptor.add("Input", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_ShaderResourceDescriptor.createLayout(m_pDevice);
    }

    virtual void _destroyV() override
    {
        destroyAndClear(m_pPlaceholderImage);
        m_Sampler.destroy();
        m_FragUbufferSet.destroyAndClearAll();
    }

    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override
    {
        VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
        DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencilInfo.depthTestEnable = VK_FALSE;
        DepthStencilInfo.depthWriteEnable = VK_FALSE;
        DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        DepthStencilInfo.stencilTestEnable = VK_FALSE;

        return DepthStencilInfo;
    }

    virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment) override
    {
        voBlendAttachment = {};
        voBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        voBlendAttachment.blendEnable = VK_TRUE;
        voBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        voBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        voBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        voBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        voBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        voBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override
    {
        voBinding = SPointData::getBindingDescription();
        voAttributeSet = SPointData::getAttributeDescriptionSet();
    }

private:
    void __updateUniformBuffer(uint32_t vImageIndex)
    {
        SUBOFrag UBOFrag = {};
        UBOFrag.ImageExtent = glm::uvec2(m_Extent.width, m_Extent.height);
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
};

class COutlineEdgeRenderPass : public vk::IRenderPass
{
protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createFramebuffers();
    void __createVertexBuffer();

    CPipelineEdge m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;

    std::vector<CPipelineEdge::SPointData> m_PointDataSet;
};
