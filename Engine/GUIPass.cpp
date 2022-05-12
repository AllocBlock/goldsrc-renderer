#include "GUIPass.h"
#include "Common.h"
#include "Log.h"
#include "AppInfo.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>
#include <set>

void CGUIRenderPass::beginFrame(std::string vTitle)
{
    if (m_Begined) throw "Already in a frame";
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(vTitle.c_str());
    m_Begined = true;
}

void CGUIRenderPass::endFrame()
{
    if (!m_Begined) throw "Already out of a frame";
    ImGui::End();
    ImGui::Render();
    m_Begined = false;
}

void CGUIRenderPass::_initV()
{
    IRenderPass::_initV();

    _ASSERTE(m_pWindow);

    uint32_t NumImage = static_cast<uint32_t>(m_AppInfo.ImageNum);
    __createRenderPass();
    __createDescriptorPool();

    // setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->AddFontFromFileTTF("C:/windows/fonts/simhei.ttf", 13.0f, NULL, IO.Fonts->GetGlyphRangesChineseFull());

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(m_pWindow, true);

    // init vulkan
    ImGui_ImplVulkan_InitInfo InitInfo = {};
    InitInfo.Instance = *m_AppInfo.pInstance;
    InitInfo.PhysicalDevice = *m_AppInfo.pPhysicalDevice;
    InitInfo.Device = *m_AppInfo.pDevice;
    InitInfo.QueueFamily = m_AppInfo.GraphicsQueueIndex;
    InitInfo.Queue = m_AppInfo.GraphicsQueue;
    InitInfo.PipelineCache = VK_NULL_HANDLE;
    InitInfo.DescriptorPool = m_DescriptorPool;
    InitInfo.Allocator = nullptr;
    InitInfo.MinImageCount = NumImage;
    InitInfo.ImageCount = NumImage;
    InitInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&InitInfo, get());

    // create command pool and buffers
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, NumImage, ECommandBufferLevel::PRIMARY);

    // upload font
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);
    m_Command.endSingleTimeBuffer(CommandBuffer);

    __createRecreateSources();
}

CRenderPassPort CGUIRenderPass::_getPortV()
{
    CRenderPassPort Ports;
    Ports.addOutput("Input", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    Ports.addOutput("Output", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    return Ports;
}

void CGUIRenderPass::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateSources();
    __createRecreateSources();
}

void CGUIRenderPass::_renderUIV()
{
    ImGui::Text(u8"默认GUI");
}

void CGUIRenderPass::_destroyV()
{
    if (*m_AppInfo.pDevice == VK_NULL_HANDLE) return;

    for (auto pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_Command.clear();
    __destroyDescriptorPool();

    m_pWindow = nullptr;

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CGUIRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_FramebufferSet.empty())
        __createFramebuffer();

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);


    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, { ClearValue });

    auto pDrawData = ImGui::GetDrawData();
    if (pDrawData)
        ImGui_ImplVulkan_RenderDrawData(pDrawData, CommandBuffer);

    end();
    vk::checkError(vkEndCommandBuffer(CommandBuffer));

    return { CommandBuffer };
}

void CGUIRenderPass::__createRenderPass()
{
    // create renderpass
    VkAttachmentDescription Attachment = createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, vk::EImageType::COLOR);

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments = &ColorAttachmentRef;

    VkSubpassDependency SubpassDependency = {}; // for render pass sync
    SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependency.dstSubpass = 0;
    SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.srcAccessMask = 0;
    SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = 1;
    RenderPassInfo.pAttachments = &Attachment;
    RenderPassInfo.subpassCount = 1;
    RenderPassInfo.pSubpasses = &Subpass;
    RenderPassInfo.dependencyCount = 1;
    RenderPassInfo.pDependencies = &SubpassDependency;

    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &RenderPassInfo, nullptr, _getPtr()));
}

void CGUIRenderPass::__createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> PoolSizes =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
    PoolInfo.pPoolSizes = PoolSizes.data();
    PoolInfo.maxSets = static_cast<uint32_t>(PoolSizes.size() * 1000);

    vk::checkError(vkCreateDescriptorPool(*m_AppInfo.pDevice, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CGUIRenderPass::__destroyDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(*m_AppInfo.pDevice, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void CGUIRenderPass::__createFramebuffer()
{
    uint32_t ImageNum = static_cast<uint32_t>(m_AppInfo.ImageNum);

    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pLink->getOutput("Output", i)
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CGUIRenderPass::__createRecreateSources()
{
}

void CGUIRenderPass::__destroyRecreateSources()
{
    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();
}