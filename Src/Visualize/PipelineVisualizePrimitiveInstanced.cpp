#include "PipelineVisualizePrimitiveInstanced.h"
#include <glm/ext/matrix_transform.hpp>

namespace
{
    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
    };

    struct SUBOFrag
    {
        alignas(16) glm::vec3 Eye;
    };

    struct SPushConstant
    {
        alignas(16) glm::mat4 Model;
        alignas(16) glm::vec3 Color;
    };

}


void CPipelineVisualizePrimitiveInstanced::add(const glm::vec3& vCenter, const glm::vec3& vScale,
    const glm::vec3& vColor)
{
    m_PrimitiveCenterSet.emplace_back(vCenter);
    m_PrimitiveScaleSet.emplace_back(vScale);
    m_ColorSet.emplace_back(vColor);
}

void CPipelineVisualizePrimitiveInstanced::clear()
{
    m_PrimitiveCenterSet.clear();
    m_PrimitiveScaleSet.clear();
    m_ColorSet.clear();
}

void CPipelineVisualizePrimitiveInstanced::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.Eye = vCamera->getPos();
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineVisualizePrimitiveInstanced::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    bind(vCommandBuffer, vImageIndex);

    VkDeviceSize Offsets[] = { 0 };
    if (m_VertexNum > 0)
    {
        VkBuffer Buffer = m_VertexBuffer;
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &Buffer, Offsets);

        // TODO: push constant, not actually instancing...which is faster?
        size_t PrimitiveNum = m_PrimitiveCenterSet.size();
        SPushConstant Constant;
        for (size_t i = 0; i < PrimitiveNum; ++i)
        {
            Constant.Model = glm::scale(glm::translate(glm::mat4(1.0f), m_PrimitiveCenterSet[i]), m_PrimitiveScaleSet[i]); // scale then translate
            Constant.Color = m_ColorSet[i];
            pushConstant(vCommandBuffer, VK_SHADER_STAGE_VERTEX_BIT, Constant);
            vkCmdDraw(vCommandBuffer, m_VertexNum, 1, 0, 0);
        }
    }
}

void CPipelineVisualizePrimitiveInstanced::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineVisualizePrimitiveInstanced::_getPipelineDescriptionV()
{
    // add shader search path
    std::filesystem::path ShaderDirPath = std::filesystem::path(__FILE__).parent_path() / "shaders/";
    Environment::addPathToEnviroment(ShaderDirPath);

    CPipelineDescriptor Descriptor;
    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.addPushConstant<SPushConstant>(VK_SHADER_STAGE_VERTEX_BIT);

    Descriptor.setVertShaderPath("instancedSurfaceShader.vert");
    Descriptor.setFragShaderPath("instancedSurfaceShader.frag");
    
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
    Descriptor.setEnableDepthTest(true);
    Descriptor.setEnableDepthWrite(true);
    Descriptor.setRasterCullMode(VkCullModeFlagBits::VK_CULL_MODE_NONE);

    return Descriptor;
}

void CPipelineVisualizePrimitiveInstanced::_createResourceV(size_t vImageNum)
{
    m_VertUniformBufferSet.destroyAndClearAll();
    m_FragUniformBufferSet.destroyAndClearAll();

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_VertUniformBufferSet.init(vImageNum);
    VkDeviceSize FragBufferSize = sizeof(SUBOFrag);
    m_FragUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
        m_FragUniformBufferSet[i]->create(m_pDevice, FragBufferSize);
    }

    __updateDescriptorSet();
    __updateVertexBuffer();
}

void CPipelineVisualizePrimitiveInstanced::_destroyV()
{
    m_VertexNum = 0;
    m_VertexBuffer.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();
    m_FragUniformBufferSet.destroyAndClearAll();
}

void CPipelineVisualizePrimitiveInstanced::__updateDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, *m_FragUniformBufferSet[i]);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineVisualizePrimitiveInstanced::_initPushConstantV(VkCommandBuffer vCommandBuffer)
{
    SPushConstant Data;
    Data.Model = glm::mat4(1.0f);
    vkCmdPushConstants(vCommandBuffer, m_PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Data), &Data);
}

void CPipelineVisualizePrimitiveInstanced::__updateVertexBuffer()
{
    m_pDevice->waitUntilIdle();
    m_VertexBuffer.destroy();

    const auto& Vertices = _createPrimitive();
    m_VertexNum = Vertices.size();

    VkDeviceSize BufferSize = sizeof(SPointData) * m_VertexNum;
    m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.stageFill(Vertices.data(), BufferSize);
}
