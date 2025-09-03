#include "PipelineVisCollider.h"
#include "VertexAttributeDescriptor.h"
#include "BasicMesh.h"
#include "Transform.h"

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
    };

    struct SUBOFrag
    {
        alignas(16) glm::vec3 EyePos;
    };

    struct SPushConstant
    {
        glm::mat4 Model;
        glm::mat4 NormalModel;
    };
}

void CPipelineVisCollider::updateUniformBuffer(cptr<CCamera> vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_pVertUniformBuffer->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.EyePos = vCamera->getPos();
    m_pFragUniformBuffer->update(&UBOFrag);
}

void CPipelineVisCollider::startRecord(sptr<CCommandBuffer> vCommandBuffer)
{
    m_CurCommandBuffer = vCommandBuffer;
    bind(vCommandBuffer);
    vCommandBuffer->bindVertexBuffer(m_VertexBuffer);
}

void CPipelineVisCollider::drawCollider(cptr<CComponentCollider> vCollider)
{
    _ASSERTE(m_CurCommandBuffer);

    if (!vCollider) return;

    auto pBasicCollider = std::dynamic_pointer_cast<const CColliderBasic>(vCollider);
    if (!pBasicCollider)
        throw std::runtime_error("Unsupported collider type");

    EBasicColliderType Type = pBasicCollider->getType();
    if (m_TypeVertexDataPosMap.find(Type) == m_TypeVertexDataPosMap.end())
        throw std::runtime_error("Unsupported collider type");

    SPushConstant Constant;
    Constant.Model = pBasicCollider->getTransform()->getAbsoluteModelMat4();
    Constant.NormalModel = glm::transpose(glm::inverse(Constant.Model));

    const auto& DataPos = m_TypeVertexDataPosMap.at(Type);
    m_CurCommandBuffer->pushConstant(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, Constant);
    m_CurCommandBuffer->draw(DataPos.First, DataPos.Count);
}

void CPipelineVisCollider::endRecord()
{
    m_CurCommandBuffer = nullptr;
}

CPipelineDescriptor CPipelineVisCollider::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("visColliderShader.vert");
    Descriptor.setFragShaderPath("visColliderShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false);
    Descriptor.addPushConstant<SPushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);

    return Descriptor;
}

void CPipelineVisCollider::_createV()
{
    // uniform buffer
    m_pVertUniformBuffer = make<vk::CUniformBuffer>();
    m_pVertUniformBuffer->create(m_pDevice, sizeof(SUBOVert));
    m_pFragUniformBuffer = make<vk::CUniformBuffer>();
    m_pFragUniformBuffer->create(m_pDevice, sizeof(SUBOFrag));

    __updateDescriptorSet();

    __initVertexBuffer();
}

void CPipelineVisCollider::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

void CPipelineVisCollider::_destroyV()
{
    m_VertexBuffer.destroy();
    destroyAndClear(m_pVertUniformBuffer);
    destroyAndClear(m_pFragUniformBuffer);
}

void CPipelineVisCollider::__updateDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_pVertUniformBuffer);
    WriteInfo.addWriteBuffer(1, *m_pFragUniformBuffer);
    m_ShaderResourceDescriptor.update(WriteInfo);
}

void CPipelineVisCollider::__initVertexBuffer()
{
    m_pDevice->waitUntilIdle();
    m_VertexBuffer.destroy();

    uint32_t CurOffset = 0;
    std::vector<SPointData> FullData;

    const std::vector<std::pair<EBasicColliderType, std::vector<BasicMesh::SVertex>>> BasicColliderLineSet =
    {
        {EBasicColliderType::PLANE, BasicMesh::getUnitQuadEdgeSet()},
        {EBasicColliderType::QUAD, BasicMesh::getUnitQuadEdgeSet()},
        {EBasicColliderType::CUBE, BasicMesh::getUnitCubeEdgeSet()},
        {EBasicColliderType::SPHERE, BasicMesh::getUnitSphereEdgeSet()},
    };

    for (const auto& Pair : BasicColliderLineSet)
    {
        EBasicColliderType Type = Pair.first;
        const std::vector<BasicMesh::SVertex>& Data = Pair.second;
        uint32_t Num = static_cast<uint32_t>(Data.size());
        m_TypeVertexDataPosMap[Type] = { CurOffset, Num };
        CurOffset += Num;

        for (const BasicMesh::SVertex& Vertex : Data)
            FullData.push_back({ Vertex.Pos, Vertex.Normal });
    }
    VkDeviceSize BufferSize = sizeof(SPointData) * FullData.size();

    m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.stageFill(FullData.data(), BufferSize);
}