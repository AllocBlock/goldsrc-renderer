#include "RendererTest.h"
#include "SceneCommon.h"

void CRendererTest::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(1.0, 0.0, 0.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    __createRenderPass();
    __createCommandPoolAndBuffers();
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

    vk::checkError(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    std::vector<VkClearValue> ClearValueSet(2);
    ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValueSet[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValueSet);
    m_Pipeline.recordCommand(CommandBuffer, vImageIndex);
    end();
    vk::checkError(vkEndCommandBuffer(CommandBuffer));
    return { CommandBuffer };
}

void CRendererTest::_destroyV()
{
    __destroyRecreateResources();
    vkDestroyRenderPass(*m_AppInfo.pDevice, m_RenderPass, nullptr);
    m_Command.clear();

    IRenderPass::_destroyV();
}

void CRendererTest::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = IRenderPass::createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);
    VkAttachmentDescription DepthAttachment = IRenderPass::createAttachmentDescription(m_RenderPassPosBitField, VkFormat::VK_FORMAT_D32_SFLOAT, EImageType::DEPTH);

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

    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CRendererTest::__destroyRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(*m_AppInfo.pDevice, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void CRendererTest::__createGraphicsPipeline()
{
    m_Pipeline.create(**m_AppInfo.pDevice, m_RenderPass, m_AppInfo.Extent);
}

void CRendererTest::__createCommandPoolAndBuffers()
{
    m_Command.createPool(*m_AppInfo.pDevice, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, m_AppInfo.ImageNum, ECommandBufferLevel::PRIMARY);

    vk::beginSingleTimeBufferFunc_t BeginFunc = [this]() -> VkCommandBuffer
    {
        return m_Command.beginSingleTimeBuffer();
    };
    vk::endSingleTimeBufferFunc_t EndFunc = [this](VkCommandBuffer vCommandBuffer)
    {
        m_Command.endSingleTimeBuffer(vCommandBuffer);
    };
    vk::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void CRendererTest::__createDepthResources()
{
    m_pDepthImage = vk::createDepthImage(**m_AppInfo.pDevice, m_AppInfo.Extent);
}

void CRendererTest::__createFramebuffers()
{
    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> Attachments =
        {
            m_AppInfo.TargetImageViewSet[i],
            *m_pDepthImage
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(*m_AppInfo.pDevice, m_RenderPass, Attachments, m_AppInfo.Extent);
    }
}

void CRendererTest::__createRecreateResources()
{
    __createGraphicsPipeline();
    __createDepthResources();
    __createFramebuffers();
    m_Pipeline.setImageNum(m_AppInfo.ImageNum);

    std::vector<SGoldSrcSprite> SpriteSet = 
    {
        {
            glm::vec3(0.0, 0.0, 0.0),
            glm::vec3(0.0, 0.0, 0.0),
            1.0,
            EGoldSrcSpriteType::PARALLEL,
            Common::Scene::generateBlackPurpleGrid(16, 16, 4),
        },
        {
            glm::vec3(0.0, 3.0, 0.0),
            glm::vec3(0.0, 0.0, 0.0),
            0.5,
            EGoldSrcSpriteType::PARALLEL,
            Common::Scene::generateDiagonalGradientGrid(64, 64, 255, 0, 0, 0, 255, 0),
        },
        {
            glm::vec3(0.0, 6.0, 0.0),
            glm::vec3(0.0, 0.0, 0.0),
            1.0,
            EGoldSrcSpriteType::PARALLEL_UP_RIGHT,
            Common::Scene::generateDiagonalGradientGrid(64, 64, 255, 255, 0, 0, 255, 0),
        },
        {
            glm::vec3(0.0, 9.0, 0.0),
            glm::vec3(-90.0, 0.0, 0.0),
            1.0,
            EGoldSrcSpriteType::PARALLEL_ORIENTED,
            Common::Scene::generateDiagonalGradientGrid(64, 64, 255, 255, 0, 0, 255, 255),
        },
        {
            glm::vec3(0.0, 12.0, 0.0),
            glm::vec3(-90.0, 0.0, 0.0),
            1.0,
            EGoldSrcSpriteType::ORIENTED,
            Common::Scene::generateDiagonalGradientGrid(64, 64, 0, 255, 255, 0, 0, 255),
        },
        {
            glm::vec3(0.0, 15.0, 0.0),
            glm::vec3(0.0, 0.0, 0.0),
            1.0,
            EGoldSrcSpriteType::FACING_UP_RIGHT,
            Common::Scene::generateDiagonalGradientGrid(64, 64, 0, 255, 0, 255, 0, 255),
        }
    };

    glm::vec3 Position;
    float Scale;
    EGoldSrcSpriteType Type;
    ptr<CIOImage> pImage;
    m_Pipeline.setSprites(SpriteSet);
}

void CRendererTest::__destroyRecreateResources()
{
    m_pDepthImage->destroy();

    for (auto pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();
    m_Pipeline.destroy();
}

void CRendererTest::__updateUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 EyeDirection = m_pCamera->getFront();

    m_Pipeline.updateUniformBuffer(vImageIndex, View, Proj, EyePos, EyeDirection);
}
