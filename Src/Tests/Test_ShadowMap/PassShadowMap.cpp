#include "PassShadowMap.h"
#include "Function.h"
#include "RenderPassDescriptor.h"

void CRenderPassShadowMap::exportShadowMapToFile(std::string vFileName)
{
    m_pDevice->waitUntilIdle();
    VkDeviceSize Size = m_ShadowMapExtent.width * m_ShadowMapExtent.height * 16;
    vk::CBuffer StageBuffer;
    StageBuffer.create(m_pDevice, Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
    
    __createRecreateResources();
}

void CRenderPassShadowMap::_initPortDescV(SPortDescriptor vDesc)
{
    SPortDescriptor Ports;
    Ports.addOutput("ShadowMap", { m_ShadowMapFormat, m_ShadowMapExtent, 0, EUsage::READ });
    VkFormat DepthFormat = m_pDevice->getPhysicalDevice()->getBestDepthFormat();
    Ports.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::UNDEFINED });
    return Ports;
}

CRenderPassDescriptor CRenderPassShadowMap::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("ShadowMap"),
        m_pPortSet->getOutputPort("Depth"));
}

void CRenderPassShadowMap::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassShadowMap::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    _ASSERTE(m_FramebufferSet[vImageIndex]->getAttachmentNum() == ClearValues.size());
    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_ShadowMapExtent, ClearValues);

    if (m_pVertBuffer->isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        VkBuffer VertBuffer = *m_pVertBuffer;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_PipelineShadowMap.bind(CommandBuffer, vImageIndex);
        vkCmdDraw(CommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
    }

    _end();

    return { CommandBuffer };
}

void CRenderPassShadowMap::_destroyV()
{
    __destroyRecreateResources();

    CRenderPassTempSceneBase::_destroyV();
}

void CRenderPassShadowMap::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassShadowMap::__createDepthResources()
{
    Function::createDepthImage(m_DepthImage, m_pDevice, m_ShadowMapExtent);
    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CRenderPassShadowMap::__createFramebuffer()
{
    __createShadowMapImages();
    size_t ImageNum = m_ShadowMapImageSet.size();
    m_FramebufferSet.init(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            *m_ShadowMapImageSet[i],
            m_DepthImage
        };
        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, m_ShadowMapExtent);
    }
}

void CRenderPassShadowMap::__createShadowMapImages()
{
    m_ShadowMapImageSet.init(m_pAppInfo->getImageNum());
    for (uint32_t i = 0; i < m_pAppInfo->getImageNum(); ++i)
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
        
        m_ShadowMapImageSet[i]->create(m_pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
        VkCommandBuffer CommandBuffer = vk::beginSingleTimeBuffer();
        m_ShadowMapImageSet[i]->transitionLayout(CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vk::endSingleTimeBuffer(CommandBuffer);

        m_pPortSet->setOutput("ShadowMap", *m_ShadowMapImageSet[i], i);
    }
}

void CRenderPassShadowMap::__createRecreateResources()
{
    __createDepthResources();
    if (isValid())
    {
        __createFramebuffer();
        m_PipelineShadowMap.create(m_pDevice, get(), m_ShadowMapExtent);
        m_PipelineShadowMap.setImageNum(m_ShadowMapImageSet.size());
    }
}

void CRenderPassShadowMap::__destroyRecreateResources()
{
    m_ShadowMapImageSet.destroyAndClearAll();
    m_FramebufferSet.destroyAndClearAll();
    m_PipelineShadowMap.destroy();
    m_DepthImage.destroy();
}

void CRenderPassShadowMap::__updateUniformBuffer(uint32_t vImageIndex)
{
    glm::mat4 LightVP = m_pLightCamera->getViewProjMat();

    m_PipelineShadowMap.updateUniformBuffer(vImageIndex, LightVP, m_pLightCamera->getNear(), m_pLightCamera->getFar());
}