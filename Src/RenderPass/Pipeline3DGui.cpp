#include "Pipeline3DGui.h"

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

    IPipeline::destroy();
}

void CPipelineLine::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SGuiUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineLine::recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    bind(vCommandBuffer, vImageIndex);

    VkDeviceSize Offsets[] = { 0 };
    if (m_VertexNum > 0)
    {
        VkBuffer Buffer = *m_pVertexBuffer;
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &Buffer, Offsets);
        vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
    }
}

void CPipelineLine::setObject(std::string vName, ptr<SGuiObject> vObject)
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
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    Info.primitiveRestartEnable = VK_FALSE;
    return Info;
}

void CPipelineLine::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SPointData::getBindingDescription();
    voAttributeSet = SPointData::getAttributeDescriptionSet();
}

void CPipelineLine::_createResourceV(size_t vImageNum)
{
    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SGuiUniformBufferObjectVert);
    m_VertUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = make<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
    }

    __updateDescriptorSet();
}

void CPipelineLine::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();
    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineLine::__updateDescriptorSet()
{
    for (size_t i = 0; i < m_Descriptor.getDescriptorSetNum(); ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_VertUniformBufferSet[i]);
        m_Descriptor.update(i, WriteInfo);
    }
}

void CPipelineLine::__updateVertexBuffer()
{
    m_pDevice->waitUntilIdle();
    if (m_pVertexBuffer) m_pVertexBuffer->destroy();

    m_VertexNum = 0;
    for (const auto& Pair : m_NameObjectMap)
    {
        const auto& pObject = Pair.second;
        m_VertexNum += pObject->Data.size();
    }
    if (m_VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SPointData) * m_VertexNum;

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
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(pData, BufferSize);
    }
}