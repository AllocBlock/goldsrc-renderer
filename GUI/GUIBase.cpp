#include "GUIBase.h"
#include "Common.h"
#include "Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>
#include <set>

void CGUIBase::_initV()
{
    CRenderer::_initV();

    uint32_t NumImage = static_cast<uint32_t>(m_AppInfo.TargetImageViewSet.size());

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

void CGUIBase::_recreateV()
{
    CRenderer::_recreateV();

    __destroyRecreateSources();
    __createRecreateSources();
}

void CGUIBase::_updateV(uint32_t vImageIndex)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(u8"窗口");
    ImGui::Text(u8"默认GUI");
    ImGui::End();
    ImGui::Render();
}

void CGUIBase::destroy()
{
    if (m_AppInfo.Device == VK_NULL_HANDLE) return;

    for (auto FrameBuffer : m_FrameBufferSet)
        vkDestroyFramebuffer(m_AppInfo.Device, FrameBuffer, nullptr);
    m_FrameBufferSet.clear();

    if (m_DescriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(m_AppInfo.Device, m_DescriptorPool, nullptr);
    m_Command.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CRenderer::destroy();
}

void CGUIBase::__createDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_AppInfo.Device, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

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

    ck(vkCreateDescriptorPool(m_AppInfo.Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

VkCommandBuffer CGUIBase::_requestCommandBufferV(uint32_t vImageIndex)
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

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);

    vkCmdEndRenderPass(CommandBuffer);
    ck(vkEndCommandBuffer(CommandBuffer));

    return CommandBuffer;
}

void CGUIBase::__createRecreateSources()
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
        ck(vkCreateFramebuffer(m_AppInfo.Device, &FramebufferInfo, nullptr, &m_FrameBufferSet[i]));
    }
}

void CGUIBase::__destroyRecreateSources()
{
    for (auto& FrameBuffer : m_FrameBufferSet)
        vkDestroyFramebuffer(m_AppInfo.Device, FrameBuffer, nullptr);
    m_FrameBufferSet.clear();
}