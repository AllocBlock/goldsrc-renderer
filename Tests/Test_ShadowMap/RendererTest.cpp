#include "RendererTest.h"

void CRendererTest::exportShadowMapToFile(std::string vFileName)
{
    vkDeviceWaitIdle(m_AppInfo.Device);
    VkDeviceSize Size = m_AppInfo.Extent.width * m_AppInfo.Extent.height * 16;
    vk::CBuffer StageBuffer;
    StageBuffer.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkBufferImageCopy CopyRegion = {};
    CopyRegion.bufferOffset = 0;
    CopyRegion.bufferImageHeight = 0;
    CopyRegion.bufferRowLength = 0;
    CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyRegion.imageSubresource.mipLevel = 0;
    CopyRegion.imageSubresource.baseArrayLayer = 0;
    CopyRegion.imageSubresource.layerCount = 1;
    CopyRegion.imageOffset = VkOffset3D{ 0, 0, 0 };
    CopyRegion.imageExtent = VkExtent3D{ m_AppInfo.Extent.width, m_AppInfo.Extent.height, 1 };

    VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
    m_ShadowMapImageSet[0]->copyToBuffer(CommandBuffer, CopyRegion, StageBuffer.get());
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    uint8_t* pData = new uint8_t[Size];
    StageBuffer.copyToHost(Size, pData);

    auto pImage = make<CIOImage>();
    pImage->setSize(m_AppInfo.Extent.width, m_AppInfo.Extent.height);
    pImage->setChannelNum(1);
    pImage->setData(pData);
    delete[] pData;

    pImage->writePPM(vFileName);
}

void CRendererTest::_initV()
{
    m_pLightCamera->setNear(4);
    m_pLightCamera->setFar(30);
    m_pLightCamera->setFov(90);
    m_pLightCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
    m_pLightCamera->setPos(glm::vec3(10.0, 10.0, 10.0));
    m_pLightCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(10.0, 10.0, 10.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    m_ShadowMapImageFormat = m_AppInfo.ImageFormat;

    __createRenderPassShadowMap();
    __createRenderPassLighting();
    __createCommandPoolAndBuffers();
    __createVertexBuffer();
    __createRecreateResources();
}

void CRendererTest::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

void CRendererTest::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRendererTest::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    Vulkan::checkError(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    __recordShadowMapRenderPass(CommandBuffer, vImageIndex);
    __recordLightRenderPass(CommandBuffer, vImageIndex);

    Vulkan::checkError(vkEndCommandBuffer(CommandBuffer));
    return { CommandBuffer };
}

void CRendererTest::_destroyV()
{
    __destroyRecreateResources();
    m_ShadowMapVertBuffer->destroy();
    m_pLightVertBuffer->destroy();
    __destroyRenderPasses();
    m_Command.clear();

    IRenderPass::_destroyV();
}

VkRenderPass CRendererTest::__createRenderPassGeneral(VkAttachmentDescription vColorAttachment, VkAttachmentDescription vDepthAttachment)
{
    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference DepthAttachmentRef = {};
    DepthAttachmentRef.attachment = 1;
    DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDependency, 1> SubpassDependencies = {};
    SubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependencies[0].dstSubpass = 0;
    SubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].srcAccessMask = 0;
    SubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount = 1;
    SubpassDesc.pColorAttachments = &ColorAttachmentRef;
    SubpassDesc.pDepthStencilAttachment = &DepthAttachmentRef;

    std::vector<VkSubpassDescription> SubpassDescs = { SubpassDesc };

    std::array<VkAttachmentDescription, 2> Attachments = { vColorAttachment, vDepthAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
    RenderPassInfo.subpassCount = static_cast<uint32_t>(SubpassDescs.size());
    RenderPassInfo.pSubpasses = SubpassDescs.data();
    RenderPassInfo.dependencyCount = static_cast<uint32_t>(SubpassDependencies.size());
    RenderPassInfo.pDependencies = SubpassDependencies.data();

    VkRenderPass RenderPass;
    Vulkan::checkError(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &RenderPass));
    return RenderPass;
}

void CRendererTest::__createRenderPassShadowMap()
{
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = m_ShadowMapImageFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    ColorAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription DepthAttachment = ColorAttachment;
    DepthAttachment.format = VkFormat::VK_FORMAT_D32_SFLOAT;

    m_RenderPassShadowMap = __createRenderPassGeneral(ColorAttachment, DepthAttachment);
}

void CRendererTest::__createRenderPassLighting()
{
    int RenderPassPosBitField = m_RenderPassPosBitField;
    VkAttachmentDescription ColorAttachment = IRenderPass::createAttachmentDescription(RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);
    VkAttachmentDescription DepthAttachment = IRenderPass::createAttachmentDescription(RenderPassPosBitField, VkFormat::VK_FORMAT_D32_SFLOAT, EImageType::DEPTH);

    m_RenderPassLight = __createRenderPassGeneral(ColorAttachment, DepthAttachment);
}

void CRendererTest::__destroyRenderPasses()
{
    if (m_RenderPassShadowMap != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPassShadowMap, nullptr);
        m_RenderPassShadowMap = VK_NULL_HANDLE;
    }
    if (m_RenderPassLight != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPassLight, nullptr);
        m_RenderPassLight = VK_NULL_HANDLE;
    }
}

void CRendererTest::__createGraphicsPipeline()
{
    m_PipelineShadowMap.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPassShadowMap, m_AppInfo.Extent);
    m_PipelineLight.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPassLight, m_AppInfo.Extent);
}

void CRendererTest::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.Device, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, m_AppInfo.ImageNum, ECommandBufferLevel::PRIMARY);

    Vulkan::beginSingleTimeBufferFunc_t BeginFunc = [this]() -> VkCommandBuffer
    {
        return m_Command.beginSingleTimeBuffer();
    };
    Vulkan::endSingleTimeBufferFunc_t EndFunc = [this](VkCommandBuffer vCommandBuffer)
    {
        m_Command.endSingleTimeBuffer(vCommandBuffer);
    };
    Vulkan::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void CRendererTest::__createDepthResources()
{
    m_pLightDepthImage = Vulkan::createDepthImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_AppInfo.Extent);
    m_pShadowMapDepthImage = Vulkan::createDepthImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_AppInfo.Extent, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
}

void CRendererTest::__createLightFramebuffers()
{
    size_t ImageNum = m_AppInfo.ImageNum;
    m_LightFramebufferSet.resize(ImageNum, VK_NULL_HANDLE);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        m_LightFramebufferSet[i] = make<vk::CFrameBuffer>();
        m_LightFramebufferSet[i]->create(m_AppInfo.Device, m_RenderPassLight, { m_AppInfo.TargetImageViewSet[i], m_pLightDepthImage->get() }, m_AppInfo.Extent);
    }
}

void CRendererTest::__createShadowMapFramebuffers()
{
    size_t ImageNum = m_ShadowMapImageSet.size();
    m_ShadowFramebufferSet.resize(ImageNum, VK_NULL_HANDLE);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        m_ShadowFramebufferSet[i] = make<vk::CFrameBuffer>();
        m_ShadowFramebufferSet[i]->create(m_AppInfo.Device, m_RenderPassShadowMap, { m_ShadowMapImageSet[i]->get(), m_pShadowMapDepthImage->get() }, m_AppInfo.Extent);
    }
}

void CRendererTest::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_ShadowMapPointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize ShadowMapVertBufferSize = sizeof(SShadowMapPointData) * VertexNum;
        m_ShadowMapVertBuffer = make<vk::CBuffer>();
        m_ShadowMapVertBuffer->create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, ShadowMapVertBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_ShadowMapVertBuffer->stageFill(m_ShadowMapPointDataSet.data(), ShadowMapVertBufferSize);

        VkDeviceSize LightVertBufferSize = sizeof(SLightPointData) * VertexNum;
        m_pLightVertBuffer = make<vk::CBuffer>();
        m_pLightVertBuffer->create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, LightVertBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pLightVertBuffer->stageFill(m_LightPointDataSet.data(), LightVertBufferSize);
    }
}

void CRendererTest::__createShadowMapImages()
{
    m_ShadowMapImageSet.resize(m_AppInfo.ImageNum);
    for (auto& pShadowMapImage : m_ShadowMapImageSet)
    {
        VkImageCreateInfo ImageInfo = {};
        ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageInfo.extent.width = m_AppInfo.Extent.width;
        ImageInfo.extent.height = m_AppInfo.Extent.height;
        ImageInfo.extent.depth = 1;
        ImageInfo.mipLevels = 1;
        ImageInfo.arrayLayers = 1;
        ImageInfo.format = m_ShadowMapImageFormat;
        ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vk::SImageViewInfo ViewInfo;
        ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

        pShadowMapImage = make<vk::CImage>();
        pShadowMapImage->create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
        VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
        pShadowMapImage->transitionLayout(CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        Vulkan::endSingleTimeBuffer(CommandBuffer);
    }
}

void CRendererTest::__createRecreateResources()
{
    __createGraphicsPipeline();
    __createDepthResources();
    __createLightFramebuffers();
    __createShadowMapImages();
    __createShadowMapFramebuffers();
    m_PipelineShadowMap.setImageNum(m_ShadowMapImageSet.size());

    std::vector<VkImageView> ShadowMapImageViewSet;
    for (auto pImage : m_ShadowMapImageSet)
    {
        ShadowMapImageViewSet.emplace_back(pImage->get());
    }
    m_PipelineLight.setShadowMapImageViews(ShadowMapImageViewSet);
    m_PipelineLight.setImageNum(m_AppInfo.ImageNum);
}

void CRendererTest::__destroyRecreateResources()
{
    for (auto& pShadowMapImage : m_ShadowMapImageSet)
        pShadowMapImage->destroy();

    for (auto& pFramebuffer : m_ShadowFramebufferSet)
        pFramebuffer->destroy();
    m_ShadowFramebufferSet.clear();

    m_pLightDepthImage->destroy();
    m_pShadowMapDepthImage->destroy();
    for (auto& pFramebuffer : m_LightFramebufferSet)
        pFramebuffer->destroy();
    m_LightFramebufferSet.clear();

    m_PipelineShadowMap.destroy();
    m_PipelineLight.destroy();
}

void CRendererTest::__generateScene()
{
    //__appendCube(glm::vec3(5.0, 0.0, 0.0), 1.0f);
    //__appendCube(glm::vec3(-5.0, 0.0, 0.0), 1.0f);
    //__appendCube(glm::vec3(0.0, 5.0, 0.0), 1.0f);
    //__appendCube(glm::vec3(0.0, -5.0, 0.0), 1.0f);
    //__appendCube(glm::vec3(0.0, 0.0, 5.0), 1.0f);
    //__appendCube(glm::vec3(0.0, 0.0, -5.0), 1.0f);

    // ground
    glm::vec3 Normal = glm::vec3(0.0, 0.0, 1.0);
    std::array<glm::vec3, 6> VertexSet =
    {
        glm::vec3(10,  10, 0),
        glm::vec3(10, -10, 0),
        glm::vec3(-10, -10, 0),
        glm::vec3(10,  10, 0),
        glm::vec3(-10, -10, 0),
        glm::vec3(-10,  10, 0),
    };
    
    for (auto& Vertex : VertexSet)
    {
        m_ShadowMapPointDataSet.emplace_back(SShadowMapPointData({ Vertex }));
        m_LightPointDataSet.emplace_back(SLightPointData({ Vertex, Normal }));
    }
    
    // objects
    __appendCube(glm::vec3(0.0, 0.0, 0.0), 5.0f);
    __appendCube(glm::vec3(0.0, 3.0, 0.0), 1.0f);
}

void CRendererTest::__appendCube(glm::vec3 vCenter, float vSize)
{
    /*
     *   4------5      y
     *  /|     /|      |
     * 0------1 |      |
     * | 7----|-6      -----x
     * |/     |/      /
     * 3------2      z
     */
    std::array<glm::vec3, 8> VertexSet =
    {
        glm::vec3(-1,  1,  1),
        glm::vec3( 1,  1,  1),
        glm::vec3( 1, -1,  1),
        glm::vec3(-1, -1,  1),
        glm::vec3(-1,  1, -1),
        glm::vec3( 1,  1, -1),
        glm::vec3( 1, -1, -1),
        glm::vec3(-1, -1, -1),
    };

    for (auto& Vertex : VertexSet)
        Vertex = vCenter + Vertex * vSize * 0.5f;

    const std::array<size_t, 36> IndexSet =
    {
        0, 1, 2, 0, 2, 3, // front
        5, 4, 7, 5, 7, 6, // back
        4, 5, 1, 4, 1, 0, // up
        3, 2, 6, 3, 6, 7, // down
        4, 0, 3, 4, 3, 7, // left
        1, 5, 6, 1, 6, 2  // right
    };

    std::array<glm::vec3, 6> NormalSet =
    {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(1, 0, 0),
    };

    for (size_t Index : IndexSet)
    {
        m_ShadowMapPointDataSet.emplace_back(SShadowMapPointData({ VertexSet[Index] }));
        m_LightPointDataSet.emplace_back(SLightPointData({ VertexSet[Index], NormalSet[Index / 6] }));
    }
}

void CRendererTest::__updateUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::mat4 LightVP = m_pLightCamera->getViewProjMat();

    m_PipelineShadowMap.updateUniformBuffer(vImageIndex, LightVP, m_pLightCamera->getNear(), m_pLightCamera->getFar());
    m_PipelineLight.updateUniformBuffer(vImageIndex, Model, View, Proj, LightVP, m_AppInfo.Extent.width, m_AppInfo.Extent.height);
}

void CRendererTest::__recordShadowMapRenderPass(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex)
{
    std::array<VkClearValue, 2> ClearValues = {};
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_RenderPassShadowMap;
    RenderPassBeginInfo.framebuffer = m_ShadowFramebufferSet[vImageIndex]->get();
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = m_AppInfo.Extent;
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
    RenderPassBeginInfo.pClearValues = ClearValues.data();

    vkCmdBeginRenderPass(vCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (m_ShadowMapVertBuffer->isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        size_t VertexNum = m_ShadowMapPointDataSet.size();
        VkBuffer VertBuffer = m_ShadowMapVertBuffer->get();
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_PipelineShadowMap.bind(vCommandBuffer, vImageIndex);
        vkCmdDraw(vCommandBuffer, VertexNum, 1, 0, 0);
    }

    vkCmdEndRenderPass(vCommandBuffer);
}

void CRendererTest::__recordLightRenderPass(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex)
{
    std::array<VkClearValue, 2> ClearValues = {};
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_RenderPassLight;
    RenderPassBeginInfo.framebuffer = m_LightFramebufferSet[vImageIndex]->get();
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = m_AppInfo.Extent;
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
    RenderPassBeginInfo.pClearValues = ClearValues.data();

    vkCmdBeginRenderPass(vCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (m_pLightVertBuffer->isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        size_t VertexNum = m_LightPointDataSet.size();
        VkBuffer VertBuffer = m_pLightVertBuffer->get();
        vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_PipelineLight.bind(vCommandBuffer, vImageIndex);
        vkCmdDraw(vCommandBuffer, VertexNum, 1, 0, 0);
    }

    vkCmdEndRenderPass(vCommandBuffer);
}