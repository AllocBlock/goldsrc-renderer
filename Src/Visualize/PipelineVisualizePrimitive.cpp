#include "PipelineVisualizePrimitive.h"
#include "Environment.h"

namespace
{
    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
        alignas(16) glm::mat4 Model;
    };

    struct SUBOFrag
    {
        alignas(16) glm::vec3 Eye;
    };

}

void CPipelineVisualizePrimitive::updateUniformBuffer(cptr<CCamera> vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.Model = glm::mat4(1.0f);
    m_pVertUniformBuffer->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.Eye = vCamera->getPos();
    m_pFragUniformBuffer->update(&UBOFrag);
}

void CPipelineVisualizePrimitive::recordCommandV(sptr<CCommandBuffer> vCommandBuffer)
{
    bind(vCommandBuffer);
    
    if (m_VertexNum > 0)
    {
        vCommandBuffer->bindVertexBuffer(m_VertexBuffer);
        vCommandBuffer->draw(0, m_VertexNum);
    }
}

void CPipelineVisualizePrimitive::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineVisualizePrimitive::_getPipelineDescriptionV()
{
    // add shader search path
    std::filesystem::path ShaderDirPath = std::filesystem::path(__FILE__).parent_path() / "shaders/";
    Environment::addPathToEnviroment(ShaderDirPath);

    CPipelineDescriptor Descriptor;
    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
    Descriptor.setEnableDepthTest(true);
    Descriptor.setEnableDepthWrite(true);
    Descriptor.setRasterCullMode(VkCullModeFlagBits::VK_CULL_MODE_NONE);

    return Descriptor;
}

void CPipelineVisualizePrimitive::_createV()
{
    destroyAndClear(m_pVertUniformBuffer);
    destroyAndClear(m_pFragUniformBuffer);

    // uniform buffer
    m_pVertUniformBuffer = make<vk::CUniformBuffer>();
    m_pVertUniformBuffer->create(m_pDevice, sizeof(SUBOVert));
    m_pFragUniformBuffer = make<vk::CUniformBuffer>();
    m_pFragUniformBuffer->create(m_pDevice, sizeof(SUBOFrag));

    __updateDescriptorSet();
}

void CPipelineVisualizePrimitive::_destroyV()
{
    m_VertexNum = 0;
    m_VertexBuffer.destroy();
    destroyAndClear(m_pVertUniformBuffer);
    destroyAndClear(m_pFragUniformBuffer);
}

void CPipelineVisualizePrimitive::__updateDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_pVertUniformBuffer);
    WriteInfo.addWriteBuffer(1, *m_pFragUniformBuffer);
    m_ShaderResourceDescriptor.update(WriteInfo);
}