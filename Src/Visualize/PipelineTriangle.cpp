#include "PipelineTriangle.h"

namespace
{
    struct SPointData
    {
        glm::vec3 Pos;
        glm::vec3 Normal;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
            return Descriptor.generate();
        }
    };

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

void CPipelineTriangle::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.Model = glm::mat4(1.0f);
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.Eye = vCamera->getPos();
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineTriangle::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
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

void CPipelineTriangle::add(const Visualize::Triangle& vTriangle)
{
    glm::vec3 Normal = glm::normalize(glm::cross(vTriangle.B - vTriangle.A, vTriangle.C - vTriangle.A));
    m_Triangles.emplace_back(std::make_pair(vTriangle, Normal));
    __updateVertexBuffer();
}

void CPipelineTriangle::clear()
{
    m_Triangles.clear();
    __updateVertexBuffer();
}

void CPipelineTriangle::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineTriangle::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("shaders/simpleShader.vert");
    Descriptor.setFragShaderPath("shaders/simpleShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
    Descriptor.setEnableDepthTest(true);
    Descriptor.setEnableDepthWrite(true);
    Descriptor.setRasterCullMode(VkCullModeFlagBits::VK_CULL_MODE_NONE);

    return Descriptor;
}

void CPipelineTriangle::_createResourceV(size_t vImageNum)
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
}

void CPipelineTriangle::_destroyV()
{
    m_VertexNum = 0;
    m_VertexBuffer.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();
    m_FragUniformBufferSet.destroyAndClearAll();
}

void CPipelineTriangle::__updateDescriptorSet()
{
    for (size_t i = 0; i < m_ShaderResourceDescriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, *m_FragUniformBufferSet[i]);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineTriangle::__updateVertexBuffer()
{
    m_pDevice->waitUntilIdle();
    m_VertexBuffer.destroy();

    m_VertexNum = m_Triangles.size() * 3;
    if (m_VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SPointData) * m_VertexNum;

        std::vector<glm::vec3> RawData;
        for (const auto& Pair : m_Triangles)
        {
            const auto& Tri = Pair.first;
            const auto& N = Pair.second;
            RawData.emplace_back(Tri.A);
            RawData.emplace_back(Tri.B);
            RawData.emplace_back(Tri.C);
            RawData.emplace_back(N);
            RawData.emplace_back(N);
            RawData.emplace_back(N);
        }
        _ASSERTE(BufferSize == RawData.size() * sizeof(glm::vec3));

        m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_VertexBuffer.stageFill(RawData.data(), BufferSize);
    }
}