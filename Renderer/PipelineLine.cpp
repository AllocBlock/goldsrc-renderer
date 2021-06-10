#include "PipelineLine.h"
#include "PointData.h"

struct SGuiUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
};

void CPipelineLine::destroy()
{
    m_VertexDataPack.destroy(m_Device);
    for (auto& Buffer : m_VertUniformBufferPacks)
        Buffer.destroy(m_Device);
    m_VertUniformBufferPacks.clear();

    CPipelineBase::destroy();
}

void CPipelineLine::updateDescriptorSet()
{
    for (size_t i = 0; i < m_Descriptor.getDescriptorSetNum(); ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SGuiUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} , {} }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CPipelineLine::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj)
{
    SGuiUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = vProj;
    UBOVert.View = vView;

    void* pData;
    ck(vkMapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory);
}

void CPipelineLine::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    bind(vCommandBuffer, vImageIndex);

    VkDeviceSize Offsets[] = { 0 };
    if (m_VertexNum > 0)
    {
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &m_VertexDataPack.Buffer, Offsets);
        vkCmdDraw(vCommandBuffer, m_VertexNum, 1, 0, 0);
    }
}

VkPipelineInputAssemblyStateCreateInfo CPipelineLine::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    Info.primitiveRestartEnable = VK_FALSE;
    return Info;
}

void CPipelineLine::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SSimplePointData::getBindingDescription();
    voAttributeSet = SSimplePointData::getAttributeDescriptionSet();
}

void CPipelineLine::_createResourceV(size_t vImageNum)
{
    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SGuiUniformBufferObjectVert);
    m_VertUniformBufferPacks.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        Common::createBuffer(m_PhysicalDevice, m_Device, VertBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPacks[i].Buffer, m_VertUniformBufferPacks[i].Memory);
    }
}

void CPipelineLine::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

    m_Descriptor.createLayout(m_Device);
}

void CPipelineLine::__updateVertexBuffer()
{
    vkDeviceWaitIdle(m_Device);
    m_VertexDataPack.destroy(m_Device);

    //size_t NumVertex = 0; // 12 edges
    //for (const auto& Pair : m_NameObjectMap)
    //{
    //    const auto& pObject = Pair.second;
    //    NumVertex += pObject->Data.size();
    //}
    //if (NumVertex > 0)
    //{
    //    VkDeviceSize BufferSize = sizeof(SSimplePointData) * NumVertex;

    //    // TODO: addtional copy is made. Is there better way?
    //    void* pData = new char[BufferSize];
    //    size_t Offset = 0;
    //    for (const auto& Pair : m_NameObjectMap)
    //    {
    //        const auto& pObject = Pair.second;
    //        size_t DataSize = sizeof(glm::vec3) * pObject->Data.size();
    //        memcpy(reinterpret_cast<char*>(pData) + Offset, pObject->Data.data(), DataSize);
    //        Offset += DataSize;
    //    }
    //    stageFillBuffer(pData, BufferSize, m_VertexDataPack);
    //}
}