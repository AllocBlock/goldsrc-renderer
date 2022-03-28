#include "PipelineShadowMap.h"

struct SUBOVertLight
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
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferSet[i]->get();
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUBOVertLight);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferSet[i]->get();
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUBOFragLight);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {FragBufferInfo} ,{} }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CPipelineShadowMap::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vLightViewProj, float vLightNear, float vLightFar)
{
    SUBOVertLight UBOVert = {};
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

void CPipelineShadowMap::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVertLight);
    VkDeviceSize FragBufferSize = sizeof(SUBOFragLight);
    m_VertUniformBufferSet.resize(vImageNum);
    m_FragUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = std::make_shared<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_PhysicalDevice, m_Device, VertBufferSize);
        m_FragUniformBufferSet[i] = std::make_shared<vk::CUniformBuffer>();
        m_FragUniformBufferSet[i]->create(m_PhysicalDevice, m_Device, FragBufferSize);
    }

    __updateDescriptorSet();
}

void CPipelineShadowMap::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
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

