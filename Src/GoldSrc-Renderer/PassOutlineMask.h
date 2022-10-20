#pragma once
#include "Vulkan.h"
#include "Common.h"
#include "FrameBuffer.h"
#include "Scene.h"
#include "Camera.h"
#include "ShaderResourceDescriptor.h"
#include "Image.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "UniformBuffer.h"
#include "VertexAttributeDescriptor.h"
#include "RenderPass.h"

#include <glm/glm.hpp>

struct SPointData
{
    glm::vec3 Pos;

    using PointData_t = SPointData;
    _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
        return Descriptor.generate();
    }
};

struct SUBOProjView
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
};

class CPipelineMask : public IPipeline
{
public:
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
    {
        SUBOProjView UBOVert = {};
        UBOVert.Proj = vCamera->getProjMat();
        UBOVert.View = vCamera->getViewMat();
        m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
    }

    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
    {
        if (m_VertexNum > 0)
        {
            bind(vCommandBuffer, vImageIndex);

            VkDeviceSize Offsets[] = { 0 };
            VkBuffer Buffer = m_VertexBuffer;
            vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &Buffer, Offsets);
            vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
        }
    }

    void setObject(ptr<CMeshDataGoldSrc> vObject)
    {
        __updateVertexBuffer(vObject);
    }

    void removeObject()
    {
        __updateVertexBuffer(nullptr);
    }

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/outlineMaskShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/outlineMaskShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override
    {
        m_VertUniformBufferSet.destroyAndClearAll();

        // uniform buffer
        VkDeviceSize VertBufferSize = sizeof(SUBOProjView);
        m_VertUniformBufferSet.init(vImageNum);

        for (size_t i = 0; i < vImageNum; ++i)
        {
            m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
        }

        __updateDescriptorSet();
    }

    virtual void _initShaderResourceDescriptorV() override
    {
        _ASSERTE(m_pDevice != VK_NULL_HANDLE);
        m_ShaderResourceDescriptor.clear();
        m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
        m_ShaderResourceDescriptor.createLayout(m_pDevice);
    }

    virtual void _destroyV() override
    {
        m_VertexNum = 0;
        m_VertexBuffer.destroy();
        m_VertUniformBufferSet.destroyAndClearAll();
    }

    virtual VkPipelineRasterizationStateCreateInfo _getRasterizationStageInfoV()
    {
        // no culling
        VkPipelineRasterizationStateCreateInfo RasterizerInfo = IPipeline::getDefaultRasterizationStageInfo();
        RasterizerInfo.cullMode = VK_CULL_MODE_NONE;

        return RasterizerInfo;
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

    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override
    {
        voBinding = SPointData::getBindingDescription();
        voAttributeSet = SPointData::getAttributeDescriptionSet();
    }

private:
    void __updateDescriptorSet()
    {
        for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
        {
            CDescriptorWriteInfo WriteInfo;
            WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
            m_ShaderResourceDescriptor.update(i, WriteInfo);
        }
    }

    void __updateVertexBuffer(ptr<CMeshDataGoldSrc> vObject)
    {
        m_pDevice->waitUntilIdle();
        m_VertexBuffer.destroy();

        if (vObject)
        {
            auto pVertexArray = vObject->getVertexArray();
            size_t NumVertex = pVertexArray->size();
            VkDeviceSize BufferSize = sizeof(SPointData) * NumVertex;

            std::vector<SPointData> PointData(NumVertex);
            for (size_t i = 0; i < NumVertex; ++i)
                PointData[i].Pos = pVertexArray->get(i);

            m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            m_VertexBuffer.stageFill(PointData.data(), BufferSize);

            m_VertexNum = NumVertex;
        }
        else
        {
            m_VertexNum = 0;
        }
    }

    size_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
};

class COutlineMaskRenderPass : public vk::IRenderPass
{
public:
    COutlineMaskRenderPass() = default;

    ptr<CCamera> getCamera() { return m_pCamera; }
    void setCamera(ptr<CCamera> vCamera) { m_pCamera = vCamera; }

    void setHighlightObject(ptr<CMeshDataGoldSrc> vObject);
    void removeHighlight();

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __rerecordCommand();
    void __createMaskImage();
    void __createFramebuffers();

    ptr<CCamera> m_pCamera = nullptr;

    CPipelineMask m_PipelineMask;
    vk::CPointerSet<vk::CImage> m_MaskImageSet;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;

    size_t m_RerecordCommandTimes = 0;
};