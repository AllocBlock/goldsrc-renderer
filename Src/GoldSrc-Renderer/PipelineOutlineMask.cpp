#include "PipelineOutlineMask.h"

#include "ComponentMeshRenderer.h"
#include "VertexAttributeDescriptor.h"

namespace
{
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
}

void CPipelineMask::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOProjView UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineMask::recordCommand(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex)
{
    if (m_VertexNum > 0)
    {
        bind(vCommandBuffer, vImageIndex);
        vCommandBuffer->bindVertexBuffer(m_VertexBuffer);
        vCommandBuffer->draw(0, m_VertexNum);
    }
}

void CPipelineMask::setActor(CActor::Ptr vActor)
{
    __updateVertexBuffer(vActor);
}

void CPipelineMask::removeObject()
{
    __updateVertexBuffer(nullptr);
}

void CPipelineMask::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineMask::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("outlineMaskShader.vert");
    Descriptor.setFragShaderPath("outlineMaskShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();

    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);
    Descriptor.setRasterCullMode(VK_CULL_MODE_NONE);
        
    Descriptor.setEnableBlend(true);
    Descriptor.setBlendMethod(EBlendFunction::NORMAL);

    return Descriptor;
}

void CPipelineMask::_createResourceV(size_t vImageNum)
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

void CPipelineMask::_destroyV()
{
    m_VertexNum = 0;
    m_VertexBuffer.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();
}

void CPipelineMask::__updateDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineMask::__updateVertexBuffer(CActor::Ptr vActor)
{
    m_pDevice->waitUntilIdle();
    m_VertexBuffer.destroy();

    m_VertexNum = 0;

    if (!vActor) return;

    auto pTransform = vActor->getTransform();
    auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
    if (!pMeshRenderer) return;

    auto pMesh = pMeshRenderer->getMesh();
    if (!pMesh) return;

    auto MeshData = pMesh->getMeshDataV();

    auto pVertexArray = MeshData.getVertexArray();
    size_t NumVertex = pVertexArray->size();
    VkDeviceSize BufferSize = sizeof(SPointData) * NumVertex;

    std::vector<SPointData> PointData(NumVertex);
    for (size_t i = 0; i < NumVertex; ++i)
        PointData[i].Pos = pVertexArray->get(i);

    m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.stageFill(PointData.data(), BufferSize);

    m_VertexNum = uint32_t(NumVertex);
}
