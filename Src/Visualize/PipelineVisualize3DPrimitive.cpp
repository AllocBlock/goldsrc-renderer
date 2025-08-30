#include "PipelineVisualize3DPrimitive.h"
#include "Environment.h"
#include <glm/ext/matrix_transform.hpp>

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
        alignas(16) glm::vec3 Eye;
    };

    struct SPushConstant
    {
        alignas(16) glm::mat4 Model;
        alignas(16) glm::vec3 Color;
    };

}


void CPipelineVisualize3DPrimitive::add(E3DPrimitiveType vPrimitiveType, const glm::vec3& vCenter, const glm::vec3& vScale,
    const glm::vec3& vColor)
{
    _ASSERTE(m_PrimitiveDataInfoMap.find(vPrimitiveType) != m_PrimitiveDataInfoMap.end());
    if (m_Primitives.find(vPrimitiveType) == m_Primitives.end())
        m_Primitives[vPrimitiveType] = { {vCenter, vScale, vColor} };
    else
        m_Primitives[vPrimitiveType].push_back({ vCenter, vScale, vColor });
}

void CPipelineVisualize3DPrimitive::clear()
{
    m_Primitives.clear();
}

void CPipelineVisualize3DPrimitive::updateUniformBuffer(CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_pVertUniformBuffer->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.Eye = vCamera->getPos();
    m_pFragUniformBuffer->update(&UBOFrag);
}

void CPipelineVisualize3DPrimitive::recordCommand(CCommandBuffer::Ptr vCommandBuffer)
{
    bind(vCommandBuffer);
    
    if (m_VertexNum > 0)
    {
        vCommandBuffer->bindVertexBuffer(m_VertexBuffer);

        // TODO: push constant, not actually instancing...which is faster?
        SPushConstant Constant;
        for (const auto& Pair : m_Primitives)
        {
            E3DPrimitiveType Type = Pair.first;
            const auto& PrimitiveSet = Pair.second;
            const SPrimitiveDataInfo DataInfo = m_PrimitiveDataInfoMap[Type];
            for (const auto& Primitive : PrimitiveSet)
            {
                Constant.Model = glm::scale(glm::translate(glm::mat4(1.0f), Primitive.Center), Primitive.Scale); // scale then translate
                Constant.Color = Primitive.Color;
                vCommandBuffer->pushConstant(VK_SHADER_STAGE_VERTEX_BIT, Constant);
                vCommandBuffer->draw(DataInfo.Start, DataInfo.Count);
            }
        }
    }
}

void CPipelineVisualize3DPrimitive::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();
    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineVisualize3DPrimitive::_getPipelineDescriptionV()
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

void CPipelineVisualize3DPrimitive::_createV()
{
    destroyAndClear(m_pVertUniformBuffer);
    destroyAndClear(m_pFragUniformBuffer);

    // uniform buffer
    m_pVertUniformBuffer = make<vk::CUniformBuffer>();
    m_pVertUniformBuffer->create(m_pDevice, sizeof(SUBOVert));
    m_pFragUniformBuffer = make<vk::CUniformBuffer>();
    m_pFragUniformBuffer->create(m_pDevice, sizeof(SUBOFrag));

    __updateDescriptorSet();
    __createVertexBuffer();
}

void CPipelineVisualize3DPrimitive::_destroyV()
{
    m_VertexNum = 0;
    m_VertexBuffer.destroy();
    destroyAndClear(m_pVertUniformBuffer);
    destroyAndClear(m_pFragUniformBuffer);
}

void CPipelineVisualize3DPrimitive::__updateDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_pVertUniformBuffer);
    WriteInfo.addWriteBuffer(1, *m_pFragUniformBuffer);
    m_ShaderResourceDescriptor.update(WriteInfo);
}

void CPipelineVisualize3DPrimitive::_initPushConstantV(CCommandBuffer::Ptr vCommandBuffer)
{
    SPushConstant Data;
    Data.Model = glm::mat4(1.0f);
    vCommandBuffer->pushConstant(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, Data);
}

void __generateSphereHelper(std::vector<SPointData>& vioVertexSet, const std::array<glm::vec3, 3>& vFace, uint32_t vLeftDepth)
{
    if (vLeftDepth == 0)
    {
        for (int i = 0; i < 3; ++i)
            vioVertexSet.push_back({ vFace[i], vFace[i] });
    }
    else
    {
        const glm::vec3& A = vFace[0];
        const glm::vec3& B = vFace[1];
        const glm::vec3& C = vFace[2];
        const glm::vec3 D = glm::normalize(vFace[0] + vFace[1]);
        const glm::vec3 E = glm::normalize(vFace[1] + vFace[2]);
        const glm::vec3 F = glm::normalize(vFace[2] + vFace[0]);
        __generateSphereHelper(vioVertexSet, { A, D, F }, vLeftDepth - 1);
        __generateSphereHelper(vioVertexSet, { D, B, E }, vLeftDepth - 1);
        __generateSphereHelper(vioVertexSet, { F, E, C }, vLeftDepth - 1);
        __generateSphereHelper(vioVertexSet, { D, E, F }, vLeftDepth - 1);
    }
}

std::vector<SPointData> __generateUnitSphere(uint32_t RecursiveDepth)
{
    glm::vec3 A = { -2.0 * sqrt(2) / 3.0, 0, -1.0 / 3.0 };
    glm::vec3 B = { sqrt(2) / 3.0, sqrt(6) / 3.0, -1.0 / 3.0 };
    glm::vec3 C = { sqrt(2) / 3.0, -sqrt(6) / 3.0, -1.0 / 3.0 };
    glm::vec3 D = { 0, 0, 1 };

    std::vector<SPointData> VertexSet;
    
    __generateSphereHelper(VertexSet, { A, B, C }, RecursiveDepth);
    __generateSphereHelper(VertexSet, { D, A, C }, RecursiveDepth);
    __generateSphereHelper(VertexSet, { D, C, B }, RecursiveDepth);
    __generateSphereHelper(VertexSet, { D, B, A }, RecursiveDepth);

    return VertexSet;
}

std::vector<SPointData> __generateUnitCube()
{
    /*
    *   4------5      y
    *  /|     /|      |
    * 0------1 |      |
    * | 7----|-6      -----x
    * |/     |/      /
    * 3------2      z
    */
    std::array<glm::vec3, 8> VertexSet =
    {
        glm::vec3(-1,  1,  1),
        glm::vec3(1,  1,  1),
        glm::vec3(1, -1,  1),
        glm::vec3(-1, -1,  1),
        glm::vec3(-1,  1, -1),
        glm::vec3(1,  1, -1),
        glm::vec3(1, -1, -1),
        glm::vec3(-1, -1, -1),
    };

    const std::array<size_t, 36> IndexSet =
    {
        0, 2, 1, 0, 3, 2, // front
        5, 7, 4, 5, 6, 7, // back
        4, 1, 5, 4, 0, 1, // up
        3, 6, 2, 3, 7, 6, // down
        4, 3, 0, 4, 7, 3, // left
        1, 6, 5, 1, 2, 6, // right
    };

    const std::array<glm::vec3, 6> NormalSet =
    {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(1, 0, 0),
    };

    std::vector<SPointData> Data(IndexSet.size());
    for (size_t i = 0; i < IndexSet.size(); ++i)
    {
        size_t Index = IndexSet[i];
        Data[i].Pos = VertexSet[Index];
        Data[i].Normal = NormalSet[i / 6];
    }
    
    return Data;
}

void CPipelineVisualize3DPrimitive::__createVertexBuffer()
{
    // generate primitive data
    size_t CurStart = 0;
    std::vector<SPointData> Data;
    // sphere
    {
        const std::vector<SPointData>& SphereData = __generateUnitSphere(4);
        size_t Count = SphereData.size();
        m_PrimitiveDataInfoMap[E3DPrimitiveType::SPHERE] = { CurStart, Count };
        CurStart += Count;
        Data.insert(Data.end(), SphereData.begin(), SphereData.end());
    }
    
    // cube
    {
        const std::vector<SPointData>& CubeData = __generateUnitCube();
        size_t Count = CubeData.size();
        m_PrimitiveDataInfoMap[E3DPrimitiveType::CUBE] = { CurStart, Count };
        CurStart += Count;
        Data.insert(Data.end(), CubeData.begin(), CubeData.end());
    }

    // create vertex buffer
    m_pDevice->waitUntilIdle();
    m_VertexBuffer.destroy();
    
    m_VertexNum = Data.size();
    VkDeviceSize BufferSize = sizeof(SPointData) * m_VertexNum;
    m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.stageFill(Data.data(), BufferSize);
}
