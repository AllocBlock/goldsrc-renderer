#include "PipelineLine.h"
#include "PointData.h"

struct SGuiUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
};

VkPipelineDepthStencilStateCreateInfo CPipelineLine::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_FALSE;
    DepthStencilInfo.depthWriteEnable = VK_FALSE;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

void CPipelineLine::destroy()
{
    m_VertexNum = 0;
    if (m_pVertexBuffer) m_pVertexBuffer->destroy();
    for (auto pBuffer : m_VertUniformBufferSet)
        pBuffer->destroy();
    m_VertUniformBufferSet.clear();

    CPipelineBase::destroy();
}

void CPipelineLine::updateDescriptorSet()
{
    for (size_t i = 0; i < m_Descriptor.getDescriptorSetNum(); ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferSet[i]->get();
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
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineLine::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    bind(vCommandBuffer, vImageIndex);

    VkDeviceSize Offsets[] = { 0 };
    if (m_VertexNum > 0)
    {
        VkBuffer Buffer = m_pVertexBuffer->get();
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &Buffer, Offsets);
        vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
    }
}

void CPipelineLine::setObject(std::string vName, std::shared_ptr<SGuiObject> vObject)
{
    m_NameObjectMap[vName] = std::move(vObject);
    __updateVertexBuffer();
}

void CPipelineLine::removeObject(std::string vName)
{
    m_NameObjectMap.erase(vName);
    __updateVertexBuffer();
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
    voBinding = SPositionPointData::getBindingDescription();
    voAttributeSet = SPositionPointData::getAttributeDescriptionSet();
}

void CPipelineLine::_createResourceV(size_t vImageNum)
{
    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SGuiUniformBufferObjectVert);
    m_VertUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = std::make_shared<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_PhysicalDevice, m_Device, VertBufferSize);
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
    m_pVertexBuffer->destroy();

    m_VertexNum = 0;
    for (const auto& Pair : m_NameObjectMap)
    {
        const auto& pObject = Pair.second;
        m_VertexNum += pObject->Data.size();
    }
    if (m_VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SPositionPointData) * m_VertexNum;

        // TODO: addtional copy is made. Is there better way?
        void* pData = new char[BufferSize];
        size_t Offset = 0;
        for (const auto& Pair : m_NameObjectMap)
        {
            const auto& pObject = Pair.second;
            size_t DataSize = sizeof(glm::vec3) * pObject->Data.size();
            memcpy(reinterpret_cast<char*>(pData) + Offset, pObject->Data.data(), DataSize);
            Offset += DataSize;
        }
        m_pVertexBuffer->fill(pData, BufferSize);
    }
}