#include "Pipeline3DGui.h"

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

    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
    };
}

void CPipelineLine::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineLine::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    bind(vCommandBuffer, vImageIndex);

    VkDeviceSize Offsets[] = { 0 };
    if (m_VertexNum > 0)
    {
        VkBuffer Buffer = m_VertexBuffer;
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &Buffer, Offsets);
        vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
    }
}

void CPipelineLine::setObject(std::string vName, ptr<SGuiObject> vObject)
{
    m_NameObjectMap[vName] = std::move(vObject);
    __updateVertexBuffer();
}

void CPipelineLine::removeObject(std::string vName)
{
    m_NameObjectMap.erase(vName);
    __updateVertexBuffer();
}

void CPipelineLine::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineLine::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("shaders/lineShader.vert");
    Descriptor.setFragShaderPath("shaders/lineShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false);
    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);

    return Descriptor;
}

void CPipelineLine::_createResourceV(size_t vImageNum)
{
    m_VertUniformBufferSet.destroyAndClearAll();

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_VertUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
    }

    __updateDescriptorSet();
}

void CPipelineLine::_destroyV()
{
    m_VertexNum = 0;
    m_VertexBuffer.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();
}

void CPipelineLine::__updateDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineLine::__updateVertexBuffer()
{
    m_pDevice->waitUntilIdle();
    m_VertexBuffer.destroy();

    m_VertexNum = 0;
    for (const auto& Pair : m_NameObjectMap)
    {
        const auto& pObject = Pair.second;
        m_VertexNum += pObject->Data.size();
    }
    if (m_VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SPointData) * m_VertexNum;

        // TODO: addtional copy is made. Is there better way?
        void* pData = new char[BufferSize];
        size_t Offset = 0;
        for (const auto& Pair : m_NameObjectMap)
        {
            const auto& pObject = Pair.second;
            size_t DataSize = sizeof(glm::vec3) * pObject->Data.size();
            memcpy(reinterpret_cast<char*>(pData) + Offset, pObject->Data.data(), DataSize);
            Offset += DataSize;
        }

        m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_VertexBuffer.stageFill(pData, BufferSize);
    }
}