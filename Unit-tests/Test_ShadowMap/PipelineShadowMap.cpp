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
        VertBufferInfo.buffer = m_VertUniformBufferPackSet[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUBOVertLight);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferPackSet[i].Buffer;
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

    void* pData;
    Vulkan::checkError(vkMapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(SUBOVertLight), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(SUBOVertLight));
    vkUnmapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory);

    SUBOFragLight UBOFrag = {};
    UBOFrag.ShadowMapCameraNear = vLightNear;
    UBOFrag.ShadowMapCameraFar = vLightFar;

    Vulkan::checkError(vkMapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(SUBOFragLight), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(SUBOFragLight));
    vkUnmapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory);
}

void CPipelineShadowMap::destroy()
{
    __destroyResources();
    CPipelineBase::destroy();
}

void CPipelineShadowMap::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SShadowMapPointData::getBindingDescription();
    voAttributeSet = SShadowMapPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineShadowMap::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

void CPipelineShadowMap::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVertLight);
    VkDeviceSize FragBufferSize = sizeof(SUBOFragLight);
    m_VertUniformBufferPackSet.resize(vImageNum);
    m_FragUniformBufferPackSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        Vulkan::createBuffer(m_PhysicalDevice, m_Device, VertBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPackSet[i].Buffer, m_VertUniformBufferPackSet[i].Memory);
        Vulkan::createBuffer(m_PhysicalDevice, m_Device, FragBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBufferPackSet[i].Buffer, m_FragUniformBufferPackSet[i].Memory);
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
    for (size_t i = 0; i < m_VertUniformBufferPackSet.size(); ++i)
    {
        m_VertUniformBufferPackSet[i].destroy(m_Device);
        m_FragUniformBufferPackSet[i].destroy(m_Device);
    }
    m_VertUniformBufferPackSet.clear();
    m_FragUniformBufferPackSet.clear();
}

