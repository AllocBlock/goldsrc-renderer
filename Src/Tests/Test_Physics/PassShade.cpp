#include "PassShade.h"
#include "Function.h"
#include "RenderPassDescriptor.h"
#include "Log.h"

std::vector<CPipelineShade::SPointData> readPointData(ptr<C3DObject> pObject)
{
    auto pVertexArray = pObject->getVertexArray();
    auto pNormalArray = pObject->getNormalArray();

    size_t NumPoint = pVertexArray->size();
    _ASSERTE(NumPoint == pNormalArray->size());

    std::vector<CPipelineShade::SPointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = pVertexArray->get(i);
        PointData[i].Normal = pNormalArray->get(i);
    }
    return PointData;
}

void CRenderPassShade::setScene(const std::vector<ptr<C3DObject>>& vObjectSet)
{
    size_t NumVertex = 0;

    for (ptr<const C3DObject> pObject : vObjectSet)
        NumVertex += pObject->getVertexArray()->size();
    if (NumVertex == 0)
    {
        Common::Log::log(u8"没有顶点数据，跳过顶点缓存创建");
        return;
    }
    m_VertexNum = NumVertex;

    VkDeviceSize BufferSize = sizeof(CPipelineShade::SPointData) * NumVertex;
    uint8_t* pData = new uint8_t[BufferSize];
    size_t Offset = 0;
    for (ptr<C3DObject> pObject : vObjectSet)
    {
        std::vector<CPipelineShade::SPointData> PointData = readPointData(pObject);
        size_t SubBufferSize = sizeof(CPipelineShade::SPointData) * pObject->getVertexArray()->size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }

    m_pVertBuffer = make<vk::CBuffer>();
    m_pVertBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertBuffer->stageFill(pData, BufferSize);
    delete[] pData;
}

void CRenderPassShade::_initV()
{
}

SPortDescriptor CRenderPassShade::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    VkFormat DepthFormat = m_AppInfo.pDevice->getPhysicalDevice()->getBestDepthFormat();
    Ports.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::UNDEFINED });
    return Ports;
}

CRenderPassDescriptor CRenderPassShade::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
        m_pPortSet->getOutputPort("Depth"));
}

void CRenderPassShade::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassShade::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValues);

    if (m_pVertBuffer->isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        VkBuffer VertBuffer = *m_pVertBuffer;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_Pipeline.bind(CommandBuffer, vImageIndex);
        vkCmdDraw(CommandBuffer, m_VertexNum, 1, 0, 0);
    }
    end();
    return { CommandBuffer };
}

void CRenderPassShade::_destroyV()
{
    __destroyRecreateResources();
    destroyAndClear(m_pVertBuffer);

    IRenderPass::_destroyV();
}

void CRenderPassShade::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassShade::__createGraphicsPipeline()
{
    m_Pipeline.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CRenderPassShade::__createDepthResources()
{
    m_DepthImage.destroy();

    VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
    Function::createDepthImage(m_DepthImage, m_AppInfo.pDevice, m_AppInfo.Extent, NULL, DepthFormat);

    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CRenderPassShade::__createFramebuffers()
{
    m_FramebufferSet.destroyAndClearAll();
    m_FramebufferSet.init(m_AppInfo.ImageNum);

    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
            m_DepthImage
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CRenderPassShade::__createRecreateResources()
{
    __createDepthResources();
    if (isValid())
    {
        __createGraphicsPipeline();
        m_Pipeline.setImageNum(m_AppInfo.ImageNum);
        __createFramebuffers();
    }
}

void CRenderPassShade::__destroyRecreateResources()
{
    m_DepthImage.destroy();
    m_FramebufferSet.destroyAndClearAll();
    m_Pipeline.destroy();
}

void CRenderPassShade::__updateUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    m_Pipeline.updateUniformBuffer(vImageIndex, m_pCamera);
}