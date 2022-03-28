#include "RendererPBR.h"
#include "UserInterface.h"
#include "Function.h"

void CRendererPBR::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(0.0, -1.0, 0.0));

    __loadSkyBox();
    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createVertexBuffer();
    __createRecreateResources();
}

void CRendererPBR::_recreateV()
{
    IRenderer::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

void CRendererPBR::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

void CRendererPBR::_renderUIV()
{
    UI::toggle("Use Color Texture", m_PipelineControl.UseColorTexture);
    UI::toggle("Use Normal Texture", m_PipelineControl.UseNormalTexture);
    UI::toggle("Use Specular Texture", m_PipelineControl.UseSpecularTexture);

    UI::split();

    UI::toggle("Force Use Material", m_PipelineControl.ForceUseMat);
    if (m_PipelineControl.ForceUseMat)
    {
        UI::indent(20.0f);
        UI::inputColor("Base Color", m_PipelineControl.Material.Albedo);
        UI::drag("Metallic", m_PipelineControl.Material.OMR.g, 0.01f, 0.0f, 1.0f);
        UI::drag("Roughness", m_PipelineControl.Material.OMR.b, 0.01f, 0.0f, 1.0f);
        UI::unindent();
    }
}

std::vector<VkCommandBuffer> CRendererPBR::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (!m_Pipeline.isReady())
        throw "Not Ready";

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    Vulkan::checkError(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    std::array<VkClearValue, 2> ClearValues = {};
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_RenderPass;
    RenderPassBeginInfo.framebuffer = m_FramebufferSet[vImageIndex]->get();
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = m_AppInfo.Extent;
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
    RenderPassBeginInfo.pClearValues = ClearValues.data();

    vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = m_pVertexBuffer->get();
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_Pipeline.bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, VertexNum, 1, 0, 0);
    }
    
    vkCmdEndRenderPass(CommandBuffer);
    Vulkan::checkError(vkEndCommandBuffer(CommandBuffer));
    return { CommandBuffer };
}

void CRendererPBR::_destroyV()
{
    __destroyRecreateResources();
    m_pVertexBuffer->destroy();
    vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
    m_Command.clear();

    IRenderer::_destroyV();
}

void CRendererPBR::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = IRenderer::createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);
    VkAttachmentDescription DepthAttachment = IRenderer::createAttachmentDescription(m_RenderPassPosBitField, VkFormat::VK_FORMAT_D32_SFLOAT, EImageType::DEPTH);

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

    Vulkan::checkError(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CRendererPBR::__loadSkyBox()
{
    if (m_SkyFilePrefix.empty()) throw "sky box image file not found";

    std::vector<std::string> Extensions = { ".tga", ".bmp", ".png", ".jpg" };

    bool FoundSkyBoxImages = false;
    for (const std::string& Extension : Extensions)
    {
        if (__readSkyboxImages(m_SkyFilePrefix, Extension))
        {
            FoundSkyBoxImages = true;
            break;
        }
    }
    if (!FoundSkyBoxImages)
        throw "sky box image file not found";
}

bool CRendererPBR::__readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension)
{
    // front back up down right left
    std::array<std::string, 6> SkyBoxPostfixes = { "ft", "bk", "up", "dn", "rt", "lf" };
    for (size_t i = 0; i < SkyBoxPostfixes.size(); ++i)
    {
        std::filesystem::path ImagePath = vSkyFilePrefix + SkyBoxPostfixes[i] + vExtension;
        if (std::filesystem::exists(ImagePath))
        {
            m_SkyBoxImageSet[i] = make<CIOImage>();
            m_SkyBoxImageSet[i]->read(ImagePath);
        }
        else
        {
            return false;
        }
    }
    return true;
}

void CRendererPBR::__destroyRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void CRendererPBR::__createGraphicsPipeline()
{
    m_Pipeline.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
}

void CRendererPBR::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.Device, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, m_AppInfo.TargetImageViewSet.size(), ECommandBufferLevel::PRIMARY);

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

void CRendererPBR::__createDepthResources()
{
    m_pDepthImage = Vulkan::createDepthImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_AppInfo.Extent);
}

void CRendererPBR::__createFramebuffers()
{
    size_t ImageNum = m_AppInfo.TargetImageViewSet.size();
    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_AppInfo.TargetImageViewSet[i],
            m_pDepthImage->get()
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.Device, m_RenderPass, AttachmentSet, m_AppInfo.Extent);
    }
}

void CRendererPBR::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SPBSPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}

void CRendererPBR::__createMaterials()
{
    CIOImage::Ptr pColorImage = make<CIOImage>("./textures/Stone_albedo.jpg");
    pColorImage->read();
    vk::CImage::Ptr pColor = Function::createImageFromIOImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pColorImage);
    m_TextureColorSet.push_back(pColor);

    CIOImage::Ptr pNormalImage = make<CIOImage>("./textures/Stone_normal.jpg");
    pNormalImage->read();
    vk::CImage::Ptr pNormal = Function::createImageFromIOImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pNormalImage);
    m_TextureNormalSet.push_back(pNormal);

    CIOImage::Ptr pSpecularImage = make<CIOImage>("./textures/Stone_omr.jpg");
    pSpecularImage->read();
    vk::CImage::Ptr pSpecular = Function::createImageFromIOImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pSpecularImage);
    m_TextureSpecularSet.push_back(pSpecular);

    _ASSERTE(m_GridSize > 0);
    size_t Num = m_GridSize * m_GridSize;

    std::vector<SMaterialPBR> MaterialSet(Num);

    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
        {
            int Index = i * 8 + j;
            MaterialSet[Index].Albedo = glm::vec4(1.0f);
            MaterialSet[Index].OMR.g = float(i) / 7.0f;
            MaterialSet[Index].OMR.b = float(j) / 7.0f;
            MaterialSet[Index].ColorIdx = 0;
            MaterialSet[Index].NormalIdx = 0;
            MaterialSet[Index].SpecularIdx = 0;
        }

    VkDeviceSize BufferSize = sizeof(SMaterialPBR) * Num;
    m_pMaterialBuffer = make<vk::CBuffer>();
    m_pMaterialBuffer->create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pMaterialBuffer->stageFill(MaterialSet.data(), BufferSize);
}

void CRendererPBR::__createRecreateResources()
{
    __createMaterials();

    __createGraphicsPipeline();
    __createDepthResources();
    __createFramebuffers();
    m_Pipeline.setImageNum(m_AppInfo.TargetImageViewSet.size());
    m_Pipeline.setSkyBoxImage(m_SkyBoxImageSet);
    m_Pipeline.setMaterialBuffer(m_pMaterialBuffer);
    m_Pipeline.setTextures(m_TextureColorSet, m_TextureNormalSet, m_TextureSpecularSet);
}

void CRendererPBR::__destroyRecreateResources()
{
    m_pDepthImage->destroy();

    for (int i = 0; i < m_TextureColorSet.size(); ++i)
        m_TextureColorSet[i]->destroy();
    m_TextureColorSet.clear();
    for (int i = 0; i < m_TextureNormalSet.size(); ++i)
        m_TextureNormalSet[i]->destroy();
    m_TextureNormalSet.clear();
    for (int i = 0; i < m_TextureSpecularSet.size(); ++i)
        m_TextureSpecularSet[i]->destroy();
    m_TextureSpecularSet.clear();

    m_pMaterialBuffer->destroy();

    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();
    m_Pipeline.destroy();
}

void CRendererPBR::__generateScene()
{
    float Sqrt2 = std::sqrt(2);
    float Sqrt3 = std::sqrt(3);
    float Sqrt6 = std::sqrt(6);
    float OneThrid = 1.0 / 3.0;
    std::array<glm::vec3, 4> VertexSet =
    {
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0, 2 * OneThrid * Sqrt2, -OneThrid),
        glm::vec3(OneThrid * Sqrt6, -OneThrid * Sqrt2, -OneThrid),
        glm::vec3(-OneThrid * Sqrt6, -OneThrid * Sqrt2, -OneThrid),
    };

    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
        {
            uint32_t Index = uint32_t(i * 8 + j);
            glm::vec3 Center = glm::vec3((i - 3.5f) * 3, 0, (j - 3.5f) * 3);

            __subdivideTriangle({ VertexSet[0], VertexSet[1], VertexSet[2] }, Center, Index, 4);
            __subdivideTriangle({ VertexSet[0], VertexSet[2], VertexSet[3] }, Center, Index, 4);
            __subdivideTriangle({ VertexSet[0], VertexSet[3], VertexSet[1] }, Center, Index, 4);
            __subdivideTriangle({ VertexSet[3], VertexSet[2], VertexSet[1] }, Center, Index, 4);
        }
    
}

void CRendererPBR::__subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, glm::vec3 vCenter, uint32_t vMaterialIndex, int vDepth)
{
    if (vDepth == 0)
    {
        for (const auto& Vertex : vVertexSet)
        {
            float u = std::atan2(Vertex.y, Vertex.x) * 0.5 / glm::pi<float>() + 0.5f;
            float v = Vertex.z * 0.5f + 0.5f;
            glm::vec2 TexCoord = glm::vec2(u, v);

            glm::vec3 Normal = glm::normalize(Vertex);
            glm::vec3 Bitangent = cross(Normal, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec3 Tangent = cross(Bitangent, Normal);

            m_PointDataSet.emplace_back(SPBSPointData({ vCenter + Vertex, Normal, Tangent, TexCoord, vMaterialIndex }));
        }
    }
    else
    {
        glm::vec3 Middle01 = glm::normalize(vVertexSet[0] + vVertexSet[1]);
        glm::vec3 Middle12 = glm::normalize(vVertexSet[1] + vVertexSet[2]);
        glm::vec3 Middle20 = glm::normalize(vVertexSet[2] + vVertexSet[0]); 

        __subdivideTriangle({ vVertexSet[0], Middle01, Middle20 }, vCenter, vMaterialIndex, vDepth - 1);
        __subdivideTriangle({ vVertexSet[1], Middle12, Middle01 }, vCenter, vMaterialIndex, vDepth - 1);
        __subdivideTriangle({ vVertexSet[2], Middle20, Middle12 }, vCenter, vMaterialIndex, vDepth - 1);
        __subdivideTriangle({ Middle01, Middle12, Middle20 }, vCenter, vMaterialIndex, vDepth - 1);
    }
}

void CRendererPBR::__updateUniformBuffer(uint32_t vImageIndex)
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

    m_Pipeline.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos, m_PipelineControl);
}
