#include "PipelineShadowMap.h"

namespace
{
    struct SUBOVert
    {
        alignas(16) glm::mat4 MVP;
    };

    struct SUBOFrag
    {
        alignas(16) float ShadowMapCameraNear;
        alignas(16) float ShadowMapCameraFar;
    };
}

void CPipelineShadowMap::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vLightViewProj, float vLightNear, float vLightFar)
{
    SUBOVert UBOVert = {};
    UBOVert.MVP = vLightViewProj;
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.ShadowMapCameraNear = vLightNear;
    UBOFrag.ShadowMapCameraFar = vLightFar;
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineShadowMap::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, *m_FragUniformBufferSet[i]);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineShadowMap::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineShadowMap::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("shaders/shadowMapVert.spv");
    Descriptor.setFragShaderPath("shaders/shadowMapFrag.spv");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setRasterFrontFace(VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE);

    return Descriptor;
}

void CPipelineShadowMap::_createV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    VkDeviceSize FragBufferSize = sizeof(SUBOFrag);
    m_VertUniformBufferSet.init(vImageNum);
    m_FragUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
        m_FragUniformBufferSet[i]->create(m_pDevice, FragBufferSize);
    }

    __updateDescriptorSet();
}

void CPipelineShadowMap::_destroyV()
{
    __destroyResources();
}

void CPipelineShadowMap::__destroyResources()
{
    m_VertUniformBufferSet.destroyAndClearAll();
    m_FragUniformBufferSet.destroyAndClearAll();
}
