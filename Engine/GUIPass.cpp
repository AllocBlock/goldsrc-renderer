﻿#include "GUIPass.h"
#include "Common.h"
#include "Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>
#include <set>

void CGUIRenderer::beginFrame(std::string vTitle)
{
    if (m_Begined) throw "Already in a frame";
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(vTitle.c_str());
    m_Begined = true;
}

void CGUIRenderer::endFrame()
{
    if (!m_Begined) throw "Already out of a frame";
    ImGui::End();
    ImGui::Render();
    m_Begined = false;
}

void CGUIRenderer::_initV()
{
    IRenderPass::_initV();

    _ASSERTE(m_pWindow);

    uint32_t NumImage = static_cast<uint32_t>(m_AppInfo.TargetImageViewSet.size());
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
    InitInfo.Instance = m_AppInfo.Instance;
    InitInfo.PhysicalDevice = m_AppInfo.PhysicalDevice;
    InitInfo.Device = m_AppInfo.Device;
    InitInfo.QueueFamily = m_AppInfo.GraphicsQueueIndex;
    InitInfo.Queue = m_AppInfo.GraphicsQueue;
    InitInfo.PipelineCache = VK_NULL_HANDLE;
    InitInfo.DescriptorPool = m_DescriptorPool;
    InitInfo.Allocator = nullptr;
    InitInfo.MinImageCount = NumImage;
    InitInfo.ImageCount = NumImage;
    InitInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&InitInfo, m_RenderPass);

    // create command pool and buffers
    m_Command.createPool(m_AppInfo.Device, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, NumImage, ECommandBufferLevel::PRIMARY);

    // upload font
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);
    m_Command.endSingleTimeBuffer(CommandBuffer);

    __createRecreateSources();
}

void CGUIRenderer::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateSources();
    __createRecreateSources();
}

void CGUIRenderer::_renderUIV()
{
    ImGui::Text(u8"默认GUI");
}

void CGUIRenderer::_destroyV()
{
    if (m_AppInfo.Device == VK_NULL_HANDLE) return;

    for (auto FrameBuffer : m_FrameBufferSet)
        vkDestroyFramebuffer(m_AppInfo.Device, FrameBuffer, nullptr);
    m_FrameBufferSet.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_Command.clear();
    __destroyDescriptorPool();
    __destroyRenderPass();

    m_pWindow = nullptr;

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CGUIRenderer::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_RenderPass;
    RenderPassBeginInfo.framebuffer = m_FrameBufferSet[vImageIndex];
    RenderPassBeginInfo.renderArea.extent = m_AppInfo.Extent;
    RenderPassBeginInfo.clearValueCount = 1;
    RenderPassBeginInfo.pClearValues = &ClearValue;
    vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto pDrawData = ImGui::GetDrawData();
    if (pDrawData)
        ImGui_ImplVulkan_RenderDrawData(pDrawData, CommandBuffer);

    vkCmdEndRenderPass(CommandBuffer);
    Vulkan::checkError(vkEndCommandBuffer(CommandBuffer));

    return { CommandBuffer };
}

void CGUIRenderer::__createRenderPass()
{
    // create renderpass
    VkAttachmentDescription Attachment = createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);

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

    Vulkan::checkError(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CGUIRenderer::__destroyRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void CGUIRenderer::__createDescriptorPool()
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

    Vulkan::checkError(vkCreateDescriptorPool(m_AppInfo.Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CGUIRenderer::__destroyDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_AppInfo.Device, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void CGUIRenderer::__createRecreateSources()
{
    uint32_t NumImage = static_cast<uint32_t>(m_AppInfo.TargetImageViewSet.size());
    // create framebuffers
    m_FrameBufferSet.resize(NumImage);
    VkFramebufferCreateInfo FramebufferInfo = {};
    FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferInfo.renderPass = m_RenderPass;
    FramebufferInfo.attachmentCount = 1;
    FramebufferInfo.width = m_AppInfo.Extent.width;
    FramebufferInfo.height = m_AppInfo.Extent.height;
    FramebufferInfo.layers = 1;
    for (uint32_t i = 0; i < NumImage; ++i)
    {
        FramebufferInfo.pAttachments = &m_AppInfo.TargetImageViewSet[i];
        Vulkan::checkError(vkCreateFramebuffer(m_AppInfo.Device, &FramebufferInfo, nullptr, &m_FrameBufferSet[i]));
    }
}

void CGUIRenderer::__destroyRecreateSources()
{
    for (auto& FrameBuffer : m_FrameBufferSet)
        vkDestroyFramebuffer(m_AppInfo.Device, FrameBuffer, nullptr);
    m_FrameBufferSet.clear();
}