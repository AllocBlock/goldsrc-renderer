#include "PipelineVisCollider.h"
#include "VertexAttributeDescriptor.h"
#include "BasicMesh.h"

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

VkPipelineDepthStencilStateCreateInfo CPipelineVisCollider::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_FALSE;
    DepthStencilInfo.depthWriteEnable = VK_FALSE;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

void CPipelineVisCollider::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.EyePos = vCamera->getPos();
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineVisCollider::startRecord(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    m_CurCommandBuffer = vCommandBuffer;
    bind(vCommandBuffer, vImageIndex);

    VkDeviceSize Offsets[] = { 0 };
    VkBuffer Buffer = m_VertexBuffer;
    vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &Buffer, Offsets);
}

void CPipelineVisCollider::draw(ICollider::CPtr vCollider)
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
    Constant.Model = pBasicCollider->getTransform()->getAbsoluteModelMat();
    Constant.NormalModel = glm::transpose(glm::inverse(Constant.Model));

    const auto& DataPos = m_TypeVertexDataPosMap.at(Type);
    pushConstant(m_CurCommandBuffer, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, Constant);
    vkCmdDraw(m_CurCommandBuffer, DataPos.Count, 1, DataPos.First, 0);
}

void CPipelineVisCollider::endRecord()
{
    m_CurCommandBuffer = VK_NULL_HANDLE;
}

VkPipelineInputAssemblyStateCreateInfo CPipelineVisCollider::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    Info.primitiveRestartEnable = VK_FALSE;
    return Info;
}

std::vector<VkPushConstantRange> CPipelineVisCollider::_getPushConstantRangeSetV()
{
    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    return { PushConstantInfo };
}

void CPipelineVisCollider::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SPointData::getBindingDescription();
    voAttributeSet = SPointData::getAttributeDescriptionSet();
}

void CPipelineVisCollider::_createResourceV(size_t vImageNum)
{
    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_VertUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
    }

    VkDeviceSize FragBufferSize = sizeof(SUBOFrag);
    m_FragUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_FragUniformBufferSet[i]->create(m_pDevice, FragBufferSize);
    }

    __updateDescriptorSet();

    __initVertexBuffer();
}

void CPipelineVisCollider::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();
    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineVisCollider::_destroyV()
{
    m_VertexBuffer.destroy();
    m_VertUniformBufferSet.destroyAndClearAll();
    m_FragUniformBufferSet.destroyAndClearAll();
}

void CPipelineVisCollider::__updateDescriptorSet()
{
    for (size_t i = 0; i < m_Descriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, *m_FragUniformBufferSet[i]);
        m_Descriptor.update(i, WriteInfo);
    }
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