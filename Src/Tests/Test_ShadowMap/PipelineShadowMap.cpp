#include "PipelineShadowMap.h"

struct SUBOVert
{
    alignas(16) glm::mat4 MVP;
};

struct SUBOFragLight
{
    alignas(16) float ShadowMapCameraNear;
    alignas(16) float ShadowMapCameraFar;
};

void CPipelineShadowMap::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, m_FragUniformBufferSet[i]);
        m_Descriptor.update(i, WriteInfo);
    }
}

void CPipelineShadowMap::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vLightViewProj, float vLightNear, float vLightFar)
{
    SUBOVert UBOVert = {};
    UBOVert.MVP = vLightViewProj;
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFragLight UBOFrag = {};
    UBOFrag.ShadowMapCameraNear = vLightNear;
    UBOFrag.ShadowMapCameraFar = vLightFar;
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineShadowMap::destroy()
{
    __destroyResources();
    IPipeline::destroy();
}

void CPipelineShadowMap::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SShadowMapPointData::getBindingDescription();
    voAttributeSet = SShadowMapPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineShadowMap::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

VkPipelineDepthStencilStateCreateInfo CPipelineShadowMap::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo Info = {};
    Info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    Info.depthTestEnable = VK_FALSE;
    Info.depthWriteEnable = VK_FALSE;
    Info.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;
    Info.depthBoundsTestEnable = VK_FALSE;
    Info.stencilTestEnable = VK_FALSE;

    return Info;
}

void CPipelineShadowMap::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    VkDeviceSize FragBufferSize = sizeof(SUBOFragLight);
    m_VertUniformBufferSet.resize(vImageNum);
    m_FragUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = make<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
        m_FragUniformBufferSet[i] = make<vk::CUniformBuffer>();
        m_FragUniformBufferSet[i]->create(m_pDevice, FragBufferSize);
    }

    __updateDescriptorSet();
}

void CPipelineShadowMap::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineShadowMap::__destroyResources()
{
    for (size_t i = 0; i < m_VertUniformBufferSet.size(); ++i)
    {
        m_VertUniformBufferSet[i]->destroy();
        m_FragUniformBufferSet[i]->destroy();
    }
    m_VertUniformBufferSet.clear();
    m_FragUniformBufferSet.clear();
}

