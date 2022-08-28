#include "PassShadowMap.h"
#include "Function.h"
#include "RenderPassDescriptor.h"

std::vector<SShadowMapPointData> readPointData(ptr<C3DObject> pObject)
{
    auto pVertexArray = pObject->getVertexArray();

    size_t NumPoint = pVertexArray->size();

    std::vector<SShadowMapPointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = pVertexArray->get(i);
    }
    return PointData;
}

void CRenderPassShadowMap::setScene(const std::vector<ptr<C3DObject>>& vObjectSet)
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

    VkDeviceSize BufferSize = sizeof(SShadowMapPointData) * NumVertex;
    uint8_t* pData = new uint8_t[BufferSize];
    size_t Offset = 0;
    for (ptr<C3DObject> pObject : vObjectSet)
    {
        std::vector<SShadowMapPointData> PointData = readPointData(pObject);
        size_t SubBufferSize = sizeof(SShadowMapPointData) * pObject->getVertexArray()->size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }

    m_pVertBuffer = make<vk::CBuffer>();
    m_pVertBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertBuffer->stageFill(pData, BufferSize);
    delete[] pData;
}

void CRenderPassShadowMap::exportShadowMapToFile(std::string vFileName)
{
    m_AppInfo.pDevice->waitUntilIdle();
    VkDeviceSize Size = m_ShadowMapExtent.width * m_ShadowMapExtent.height * 16;
    vk::CBuffer StageBuffer;
    StageBuffer.create(m_AppInfo.pDevice, Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkBufferImageCopy CopyRegion = {};
    CopyRegion.bufferOffset = 0;
    CopyRegion.bufferImageHeight = 0;
    CopyRegion.bufferRowLength = 0;
    CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyRegion.imageSubresource.mipLevel = 0;
    CopyRegion.imageSubresource.baseArrayLayer = 0;
    CopyRegion.imageSubresource.layerCount = 1;
    CopyRegion.imageOffset = VkOffset3D{ 0, 0, 0 };
    CopyRegion.imageExtent = VkExtent3D{ m_ShadowMapExtent.width, m_ShadowMapExtent.height, 1 };

    VkCommandBuffer CommandBuffer = vk::beginSingleTimeBuffer();
    m_ShadowMapImageSet[0]->copyToBuffer(CommandBuffer, CopyRegion, StageBuffer.get());
    vk::endSingleTimeBuffer(CommandBuffer);

    uint8_t* pData = new uint8_t[Size];
    StageBuffer.copyToHost(Size, pData);

    auto pImage = make<CIOImage>();
    pImage->setSize(m_ShadowMapExtent.width, m_ShadowMapExtent.height);
    pImage->setChannelNum(1);
    pImage->setData(pData);
    delete[] pData;

    pImage->writePPM(vFileName);
}

void CRenderPassShadowMap::_initV()
{
    m_pLightCamera->setNear(4);
    m_pLightCamera->setFar(30);
    m_pLightCamera->setFov(90);
    m_pLightCamera->setAspect(1.0f);
    m_pLightCamera->setPos(glm::vec3(10.0, 10.0, 10.0));
    m_pLightCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createRecreateResources();
}

SPortDescriptor CRenderPassShadowMap::_getPortDescV()
{
    CRenderPassPort Ports;
    Ports.addOutput("Output", m_ShadowMapFormat, m_ShadowMapExtent);
    return Ports;
}

void CRenderPassShadowMap::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassShadowMap::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassShadowMap::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_FramebufferSet.empty() || m_pLink->isUpdated())
    {
        __createFramebuffer();
        m_pLink->setUpdateState(false);
    }

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    std::vector<VkClearValue> ClearValues(1);
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_ShadowMapExtent, ClearValues);

    if (m_pVertBuffer->isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        VkBuffer VertBuffer = *m_pVertBuffer;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_PipelineShadowMap.bind(CommandBuffer, vImageIndex);
        vkCmdDraw(CommandBuffer, m_VertexNum, 1, 0, 0); 
    }

    end();

    return { CommandBuffer };
}

void CRenderPassShadowMap::_destroyV()
{
    __destroyRecreateResources();
    m_pVertBuffer->destroy();
    m_Command.clear();

    IRenderPass::_destroyV();
}

void CRenderPassShadowMap::__createRenderPass()
{
    // TODO: how to manage load op and initial layout?
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = m_ShadowMapFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    ColorAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    CRenderPassDescriptor Desc;
    Desc.addColorAttachment(ColorAttachment);
    auto Info = Desc.generateInfo();

    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &Info, nullptr, _getPtr()));
}

void CRenderPassShadowMap::__createGraphicsPipeline()
{
    m_PipelineShadowMap.create(m_AppInfo.pDevice, get(), m_ShadowMapExtent);
}

void CRenderPassShadowMap::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_CommandName, m_AppInfo.ImageNum, ECommandBufferLevel::PRIMARY);
}

void CRenderPassShadowMap::__createFramebuffer()
{
    size_t ImageNum = m_ShadowMapImageSet.size();
    m_FramebufferSet.resize(ImageNum, VK_NULL_HANDLE);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), { *m_ShadowMapImageSet[i] }, m_ShadowMapExtent);
    }
}

void CRenderPassShadowMap::__createShadowMapImages()
{
    m_ShadowMapImageSet.resize(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        VkImageCreateInfo ImageInfo = {};
        ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageInfo.extent.width = m_ShadowMapExtent.width;
        ImageInfo.extent.height = m_ShadowMapExtent.height;
        ImageInfo.extent.depth = 1;
        ImageInfo.mipLevels = 1;
        ImageInfo.arrayLayers = 1;
        ImageInfo.format = m_ShadowMapFormat;
        ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vk::SImageViewInfo ViewInfo;
        ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

        m_ShadowMapImageSet[i] = make<vk::CImage>();
        m_ShadowMapImageSet[i]->create(m_AppInfo.pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
        VkCommandBuffer CommandBuffer = vk::beginSingleTimeBuffer();
        m_ShadowMapImageSet[i]->transitionLayout(CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vk::endSingleTimeBuffer(CommandBuffer);

        m_pLink->linkOutput("ShadowMap", *m_ShadowMapImageSet[i], i);
    }
}

void CRenderPassShadowMap::__createRecreateResources()
{
    __createGraphicsPipeline();
    __createShadowMapImages();
    m_PipelineShadowMap.setImageNum(m_ShadowMapImageSet.size());
}

void CRenderPassShadowMap::__destroyRecreateResources()
{
    for (auto& pShadowMapImage : m_ShadowMapImageSet)
        pShadowMapImage->destroy();
    m_ShadowMapImageSet.clear();

    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();

    m_PipelineShadowMap.destroy();
}

void CRenderPassShadowMap::__updateUniformBuffer(uint32_t vImageIndex)
{
    glm::mat4 LightVP = m_pLightCamera->getViewProjMat();

    m_PipelineShadowMap.updateUniformBuffer(vImageIndex, LightVP, m_pLightCamera->getNear(), m_pLightCamera->getFar());
}