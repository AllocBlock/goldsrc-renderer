#include "PassShade.h"
#include "Function.h"
#include "RenderPassDescriptor.h"
#include "ShadowMapDefines.h"

std::vector<SPointData> readPointData(ptr<CGeneralMeshData> pObject)
{
    auto pVertexArray = pObject->getVertexArray();
    auto pNormalArray = pObject->getNormalArray();

    size_t NumPoint = pVertexArray->size();
    _ASSERTE(NumPoint == pNormalArray->size());

    std::vector<SPointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = pVertexArray->get(i);
        PointData[i].Normal = pNormalArray->get(i);
    }
    return PointData;
}


void CRenderPassShade::setShadowMapInfo(CCamera::CPtr vLightCamera)
{
    _ASSERTE(vLightCamera);
    m_pLightCamera = vLightCamera;
}

void CRenderPassShade::setScene(const std::vector<ptr<CGeneralMeshData>>& vObjectSet)
{
    size_t NumVertex = 0;

    for (ptr<const CGeneralMeshData> pObject : vObjectSet)
        NumVertex += pObject->getVertexArray()->size();
    if (NumVertex == 0)
    {
        Common::Log::log(u8"没有顶点数据，跳过顶点缓存创建");
        return;
    }
    m_VertexNum = NumVertex;

    VkDeviceSize BufferSize = sizeof(SPointData) * NumVertex;
    uint8_t* pData = new uint8_t[BufferSize];
    size_t Offset = 0;
    for (ptr<CGeneralMeshData> pObject : vObjectSet)
    {
        std::vector<SPointData> PointData = readPointData(pObject);
        size_t SubBufferSize = sizeof(SPointData) * pObject->getVertexArray()->size();
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
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(10.0, 10.0, 10.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    __createRenderPass();
    __createRecreateResources();
}

SPortDescriptor CRenderPassShade::_getPortDescV()
{
    CRenderPassPort Ports;
    // TODO: convertable image format matching
    VkExtent2D ShadowMapExtent = { gShadowMapSize, gShadowMapSize };
    Ports.addInput("ShadowMap", gShadowMapImageFormat, ShadowMapExtent);
    Ports.addOutput("Main", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    return Ports;
}

void CRenderPassShade::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassShade::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassShade::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_FramebufferSet.empty() || m_pLink->isUpdated())
    {
        __createLightFramebuffers();

        std::vector<VkImageView> ShadowMapImageViewSet;
        for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
        {
            ShadowMapImageViewSet.emplace_back(m_pLink->getInput("ShadowMap", i));
        }

        m_Pipeline.setShadowMapImageViews(ShadowMapImageViewSet);
        m_pLink->setUpdateState(false);
    }

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

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
    m_pVertBuffer->destroy();
    m_pVertexBuffer = nullptr;

    IRenderPass::_destroyV();
}

void CRenderPassShade::__createRenderPass()
{
    auto Info = CRenderPassDescriptor::generateSingleSubpassInfo(m_RenderPassPosBitField, m_AppInfo.ImageFormat, VK_FORMAT_D32_SFLOAT);
    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &Info, nullptr, _getPtr()));
}

void CRenderPassShade::__createGraphicsPipeline()
{
    m_Pipeline.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CRenderPassShade::__createDepthResources()
{
    m_pDepthImage = Function::createDepthImage(m_AppInfo.pDevice, m_AppInfo.Extent);
}

void CRenderPassShade::__createLightFramebuffers()
{
    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.resize(ImageNum, VK_NULL_HANDLE);

    std::vector<VkImageView> ShadowMapImageViews(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), { m_pLink->getOutput("Main", i), *m_pDepthImage}, m_AppInfo.Extent);

        ShadowMapImageViews.emplace_back(m_pLink->getInput("ShadowMap", i));
    }
}

void CRenderPassShade::__createRecreateResources()
{
    __createGraphicsPipeline();
    __createDepthResources();
    m_Pipeline.setImageNum(m_AppInfo.ImageNum);
}

void CRenderPassShade::__destroyRecreateResources()
{
    m_pDepthImage->destroy();
    for (auto pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();
    m_Pipeline.destroy();
}

void CRenderPassShade::__updateUniformBuffer(uint32_t vImageIndex)
{
    _ASSERTE(m_pLightCamera);

    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    m_Pipeline.updateUniformBuffer(vImageIndex, m_pCamera, m_pLightCamera, gShadowMapSize);
}