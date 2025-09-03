#include "PipelineVisCollidePoint.h"
#include "VertexAttributeDescriptor.h"
#include "Transform.h"

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

    struct SPushConstant
    {
        glm::mat4 Model;
    };
}

void CPipelineVisCollidePoint::addCollidePoint(glm::vec3 vPos, glm::vec3 vNormal, float vShowTime)
{
    m_CollidePointInfoSet.push_back({ vPos, vNormal, vShowTime });
}

void CPipelineVisCollidePoint::updateUniformBuffer(cptr<CCamera> vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_pVertUniformBuffer->update(&UBOVert);
}

void CPipelineVisCollidePoint::record(sptr<CCommandBuffer> vCommandBuffer, glm::vec3 vEyePos)
{
    bind(vCommandBuffer);
    vCommandBuffer->bindVertexBuffer(m_VertexBuffer);

    SPushConstant Constant;
    CTransform Transform;

    float DeltaTime = m_Ticker.update();
    for (SCollidePointInfo& Info : m_CollidePointInfoSet)
    {
        Transform.setTranslate(Info.Pos);
        Transform.setRotate(CRotator::createVectorToVector(glm::vec3(0, 0, 1), Info.Normal));
        Constant.Model = Transform.getAbsoluteModelMat4();

        vCommandBuffer->pushConstant(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, Constant);
        vCommandBuffer->draw(0, m_VertexNum);

        Info.LeftTime -= DeltaTime;
    }

    // clear timeout points
    while(!m_CollidePointInfoSet.empty() && m_CollidePointInfoSet[0].LeftTime <= 0)
    {
        m_CollidePointInfoSet.erase(m_CollidePointInfoSet.begin());
    }
}

CPipelineDescriptor CPipelineVisCollidePoint::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("visCollidePointShader.vert");
    Descriptor.setFragShaderPath("visCollidePointShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false);
    Descriptor.addPushConstant<SPushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);

    return Descriptor;
}

void CPipelineVisCollidePoint::_createV()
{
    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_pVertUniformBuffer = make<vk::CUniformBuffer>();
    m_pVertUniformBuffer->create(m_pDevice, VertBufferSize);

    __updateDescriptorSet();

    __initVertexBuffer();
}

void CPipelineVisCollidePoint::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

void CPipelineVisCollidePoint::_destroyV()
{
    m_VertexBuffer.destroy();
    destroyAndClear(m_pVertUniformBuffer);
}

void CPipelineVisCollidePoint::__updateDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_pVertUniformBuffer);
    m_ShaderResourceDescriptor.update(WriteInfo);
}

void CPipelineVisCollidePoint::__initVertexBuffer()
{
    m_pDevice->waitUntilIdle();
    m_VertexBuffer.destroy();

    std::vector<SPointData> LineSet;

    // circle
    const int SegNum = 16;
    const float Radius = 0.1f;
    for (int i = 0; i < SegNum; ++i)
    {
        float Rad1 = (i / static_cast<float>(SegNum)) * glm::pi<float>() * 2.0f;
        float Rad2 = (i + 1) / static_cast<float>(SegNum) * glm::pi<float>() * 2.0f;
        LineSet.push_back({glm::vec3(glm::cos(Rad1), glm::sin(Rad1), 0.0f) * Radius});
        LineSet.push_back({glm::vec3(glm::cos(Rad2), glm::sin(Rad2), 0.0f) * Radius});
    }

    // arrow
    const float Length = 0.3f;
    const glm::vec3 Origin = glm::vec3(0.0f, 0.0f, 0.0f);
    const glm::vec3 Head = glm::vec3(0.0f, 0.0f, Length);
    const glm::vec3 LeftEar = Head - glm::vec3(Length * 0.2f, 0.0f, Length * 0.3f);
    const glm::vec3 RightEar = Head - glm::vec3(-Length * 0.2f, 0.0f, Length * 0.3f);
    LineSet.push_back({ Origin });
    LineSet.push_back({ Head });
    LineSet.push_back({ Head });
    LineSet.push_back({ LeftEar });
    LineSet.push_back({ Head });
    LineSet.push_back({ RightEar });

    m_VertexNum = static_cast<uint32_t>(LineSet.size());
    VkDeviceSize BufferSize = sizeof(SPointData) * m_VertexNum;

    m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.stageFill(LineSet.data(), BufferSize);
}