#include "GUIBase.h"
#include "Common.h"
#include "Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>
#include <set>

void CGUIBase::init(GLFWwindow* vWindow, const Common::SVulkanAppInfo& vAppInfo, bool vHasRenderer)
{
    m_pWindow = vWindow;
    m_PhysicalDevice = vAppInfo.PhysicalDevice;
    m_Device = vAppInfo.Device;
    m_GraphicsQueueIndex = vAppInfo.GraphicsQueueIndex;
    m_GraphicsQueue = vAppInfo.GraphicsQueue;
    m_Extent = vAppInfo.Extent;
    m_ImageFormat = vAppInfo.ImageForamt;
    m_TargetImageViewSet = vAppInfo.TargetImageViewSet;
    uint32_t NumImage = static_cast<uint32_t>(m_TargetImageViewSet.size());

    __createDescriptorPool();

    // setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->AddFontFromFileTTF("C:/windows/fonts/simhei.ttf", 13.0f, NULL, IO.Fonts->GetGlyphRangesChineseFull());

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(m_pWindow, true);

    // create renderpass
    VkAttachmentDescription Attachment = {};
    Attachment.format = m_ImageFormat;
    Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    Attachment.loadOp = (vHasRenderer ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR);
    Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachment.initialLayout = (vHasRenderer ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED);
    Attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
    ck(vkCreateRenderPass(m_Device, &RenderPassInfo, nullptr, &m_RenderPass));

    // init vulkan
    ImGui_ImplVulkan_InitInfo InitInfo = {};
    InitInfo.Instance = vAppInfo.Instance;
    InitInfo.PhysicalDevice = m_PhysicalDevice;
    InitInfo.Device = m_Device;
    InitInfo.QueueFamily = m_GraphicsQueueIndex;
    InitInfo.Queue = m_GraphicsQueue;
    InitInfo.PipelineCache = VK_NULL_HANDLE;
    InitInfo.DescriptorPool = m_DescriptorPool;
    InitInfo.Allocator = nullptr;
    InitInfo.MinImageCount = NumImage;
    InitInfo.ImageCount = NumImage;
    InitInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&InitInfo, m_RenderPass);

    // create command pool and buffers
    m_Command.createPool(m_Device, ECommandType::RESETTABLE, m_GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, NumImage, ECommandBufferLevel::PRIMARY);

    // upload font
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);
    m_Command.endSingleTimeBuffer(CommandBuffer);

    __createRecreateSources();

    _initV();
}

void CGUIBase::recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    m_Extent = vExtent;
    m_ImageFormat = vImageFormat;
    m_TargetImageViewSet = vImageViews;

    __destroyRecreateSources();
    __createRecreateSources();
}

void CGUIBase::destroy()
{
    if (m_Device == VK_NULL_HANDLE) return;

    for (auto FrameBuffer : m_FrameBufferSet)
        vkDestroyFramebuffer(m_Device, FrameBuffer, nullptr);
    m_FrameBufferSet.clear();

    if (m_RenderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    if (m_DescriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    m_Command.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_pWindow = nullptr;
    m_PhysicalDevice = VK_NULL_HANDLE;
    m_Device = VK_NULL_HANDLE;
    m_DescriptorPool = VK_NULL_HANDLE;
    m_RenderPass = VK_NULL_HANDLE;
    m_Command = CCommand();
    m_CommandName = "Main";
    m_ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
    m_TargetImageViewSet;
    m_Extent = { 0, 0 };

    m_GraphicsQueueIndex = 0;
    m_GraphicsQueue = VK_NULL_HANDLE;
}

void CGUIBase::_initV()
{
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

void CGUIBase::__createDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
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

    ck(vkCreateDescriptorPool(m_Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CGUIBase::__createRecreateSources()
{
    uint32_t NumImage = static_cast<uint32_t>(m_TargetImageViewSet.size());
    // create framebuffers
    m_FrameBufferSet.resize(NumImage);
    VkFramebufferCreateInfo FramebufferInfo = {};
    FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferInfo.renderPass = m_RenderPass;
    FramebufferInfo.attachmentCount = 1;
    FramebufferInfo.width = m_Extent.width;
    FramebufferInfo.height = m_Extent.height;
    FramebufferInfo.layers = 1;
    for (uint32_t i = 0; i < NumImage; ++i)
    {
        FramebufferInfo.pAttachments = &m_TargetImageViewSet[i];
        ck(vkCreateFramebuffer(m_Device, &FramebufferInfo, nullptr, &m_FrameBufferSet[i]));
    }
}

void CGUIBase::__destroyRecreateSources()
{
    for (auto& FrameBuffer : m_FrameBufferSet)
        vkDestroyFramebuffer(m_Device, FrameBuffer, nullptr);
    m_FrameBufferSet.clear();
}

VkCommandBuffer CGUIBase::requestCommandBuffer(uint32_t vImageIndex)
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
    RenderPassBeginInfo.renderArea.extent = m_Extent;
    RenderPassBeginInfo.clearValueCount = 1;
    RenderPassBeginInfo.pClearValues = &ClearValue;
    vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);

    vkCmdEndRenderPass(CommandBuffer);
    ck(vkEndCommandBuffer(CommandBuffer));

    return CommandBuffer;
}

void CGUIBase::update(uint32_t vImageIndex)
{
    _updateV(vImageIndex);
}