#include "RendererTest.h"

void CRendererTest::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(0.0, -1.0, 0.0));

    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createVertexBuffer();
    __createRecreateResources();
}

void CRendererTest::_recreateV()
{
    CRenderer::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

void CRendererTest::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

VkCommandBuffer CRendererTest::_requestCommandBufferV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    std::array<VkClearValue, 2> ClearValues = {};
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_RenderPass;
    RenderPassBeginInfo.framebuffer = m_FramebufferSet[vImageIndex];
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = m_AppInfo.Extent;
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
    RenderPassBeginInfo.pClearValues = ClearValues.data();

    vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (m_VertexBufferPack.isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &m_VertexBufferPack.Buffer, Offsets);
        m_Pipeline.bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, VertexNum, 1, 0, 0);
    }
    
    vkCmdEndRenderPass(CommandBuffer);
    ck(vkEndCommandBuffer(CommandBuffer));
    return CommandBuffer;
}

void CRendererTest::_destroyV()
{
    __destroyRecreateResources();
    m_VertexBufferPack.destroy(m_AppInfo.Device);
    vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
    m_Command.clear();

    CRenderer::_destroyV();
}

void CRendererTest::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = CRenderer::createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);
    VkAttachmentDescription DepthAttachment = CRenderer::createAttachmentDescription(m_RenderPassPosBitField, VkFormat::VK_FORMAT_D32_SFLOAT, EImageType::DEPTH);

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

    std::array<VkAttachmentDescription, 2> Attachments = { ColorAttachment, DepthAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
    RenderPassInfo.subpassCount = static_cast<uint32_t>(SubpassDescs.size());
    RenderPassInfo.pSubpasses = SubpassDescs.data();
    RenderPassInfo.dependencyCount = static_cast<uint32_t>(SubpassDependencies.size());
    RenderPassInfo.pDependencies = SubpassDependencies.data();

    ck(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CRendererTest::__destroyRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void CRendererTest::__createGraphicsPipeline()
{
    m_Pipeline.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
}

void CRendererTest::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.Device, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, m_AppInfo.TargetImageViewSet.size(), ECommandBufferLevel::PRIMARY);

    Common::beginSingleTimeBufferFunc_t BeginFunc = [this]() -> VkCommandBuffer
    {
        return m_Command.beginSingleTimeBuffer();
    };
    Common::endSingleTimeBufferFunc_t EndFunc = [this](VkCommandBuffer vCommandBuffer)
    {
        m_Command.endSingleTimeBuffer(vCommandBuffer);
    };
    Common::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void CRendererTest::__createDepthResources()
{
    VkFormat DepthFormat = VkFormat::VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = m_AppInfo.Extent.width;
    ImageInfo.extent.height = m_AppInfo.Extent.height;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = DepthFormat;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Common::createImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImagePack.Image, m_DepthImagePack.Memory);
    m_DepthImagePack.ImageView = Common::createImageView(m_AppInfo.Device, m_DepthImagePack.Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    Common::transitionImageLayout(CommandBuffer, m_DepthImagePack.Image, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    m_Command.endSingleTimeBuffer(CommandBuffer);
}

void CRendererTest::__createFramebuffers()
{
    size_t ImageNum = m_AppInfo.TargetImageViewSet.size();
    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::array<VkImageView, 2> Attachments =
        {
            m_AppInfo.TargetImageViewSet[i],
            m_DepthImagePack.ImageView
        };

        VkFramebufferCreateInfo FramebufferInfo = {};
        FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass = m_RenderPass;
        FramebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
        FramebufferInfo.pAttachments = Attachments.data();
        FramebufferInfo.width = m_AppInfo.Extent.width;
        FramebufferInfo.height = m_AppInfo.Extent.height;
        FramebufferInfo.layers = 1;

        ck(vkCreateFramebuffer(m_AppInfo.Device, &FramebufferInfo, nullptr, &m_FramebufferSet[i]));
    }
}

void CRendererTest::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(STestPointData) * VertexNum;
        Common::stageFillBuffer(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_PointDataSet.data(), BufferSize, m_VertexBufferPack.Buffer, m_VertexBufferPack.Memory);
    }
}

void CRendererTest::__createRecreateResources()
{
    __createGraphicsPipeline();
    __createDepthResources();
    __createFramebuffers();
    m_Pipeline.setImageNum(m_AppInfo.TargetImageViewSet.size());
}

void CRendererTest::__destroyRecreateResources()
{
    m_DepthImagePack.destroy(m_AppInfo.Device);

    for (auto& Framebuffer : m_FramebufferSet)
        vkDestroyFramebuffer(m_AppInfo.Device, Framebuffer, nullptr);
    m_FramebufferSet.clear();
    m_Pipeline.destroy();
}

void CRendererTest::__generateScene()
{
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
        m_PointDataSet.emplace_back(STestPointData({ VertexSet[Index], NormalSet[Index / 6] }));
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
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    m_Pipeline.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
}
