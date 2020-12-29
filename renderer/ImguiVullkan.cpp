#include "ImguiVullkan.h"
#include "Common.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

CImguiVullkan::CImguiVullkan(VkInstance vInstance, VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, uint32_t vGraphicQueueFamily, VkQueue vGraphicQueue, GLFWwindow* vpWindow, VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    m_Instance = vInstance;
    m_PhysicalDevice = vPhysicalDevice;
    m_Device = vDevice;
    m_GraphicsQueueFamily = vGraphicQueueFamily;
    m_GraphicsQueue = vGraphicQueue;
    m_pWindow = vpWindow;
    m_ImageFormat = vImageFormat;
    m_Extent = vExtent;
    __createDescriptorPool();

    uint32_t NumImage = static_cast<uint32_t>(vImageViews.size());
    // setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(vpWindow, true);

    // create renderpass
    VkAttachmentDescription Attachment = {};
    Attachment.format = vImageFormat;
    Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
    InitInfo.Instance = m_Instance;
    InitInfo.PhysicalDevice = m_PhysicalDevice;
    InitInfo.Device = m_Device;
    InitInfo.QueueFamily = m_GraphicsQueueFamily;
    InitInfo.Queue = m_GraphicsQueue;
    InitInfo.PipelineCache = VK_NULL_HANDLE;
    InitInfo.DescriptorPool = m_DescriptorPool;
    InitInfo.Allocator = nullptr;
    InitInfo.MinImageCount = vImageViews.size();
    InitInfo.ImageCount = vImageViews.size();
    InitInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&InitInfo, m_RenderPass);

    // create command pool
    VkCommandPoolCreateInfo CommandPoolInfo = {};
    CommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CommandPoolInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    CommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ck(vkCreateCommandPool(m_Device, &CommandPoolInfo, nullptr, &m_CommandPool));

    // create command buffers
    m_CommandBuffers.resize(NumImage);
    VkCommandBufferAllocateInfo CommandBufferInfo = {};
    CommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferInfo.commandPool = m_CommandPool;
    CommandBufferInfo.commandBufferCount = NumImage;
    ck(vkAllocateCommandBuffers(m_Device, &CommandBufferInfo, m_CommandBuffers.data()));

    // upload font
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);
    ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);
    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);

    updateFramebuffers(m_Extent, vImageViews);
}

CImguiVullkan::~CImguiVullkan()
{
    for (auto Framebuffer : m_FrameBuffers)
        vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);

    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void CImguiVullkan::__createDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
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

void CImguiVullkan::updateFramebuffers(VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    m_Extent = vExtent;
    for (auto Framebuffer : m_FrameBuffers)
        vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);

    uint32_t NumImage = static_cast<uint32_t>(vImageViews.size());
    // create framebuffers
    m_FrameBuffers.clear();
    m_FrameBuffers.resize(NumImage);
    VkFramebufferCreateInfo FramebufferInfo = {};
    FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferInfo.renderPass = m_RenderPass;
    FramebufferInfo.attachmentCount = 1;
    FramebufferInfo.width = m_Extent.width;
    FramebufferInfo.height = m_Extent.height;
    FramebufferInfo.layers = 1;
    for (uint32_t i = 0; i < NumImage; ++i)
    {
        FramebufferInfo.pAttachments = &vImageViews[i];
        ck(vkCreateFramebuffer(m_Device, &FramebufferInfo, nullptr, &m_FrameBuffers[i]));
    }
}

void CImguiVullkan::render()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
}

VkCommandBuffer CImguiVullkan::requestCommandBuffer(uint32_t vImageIndex)
{
    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_CommandBuffers[vImageIndex], &CommandBufferBeginInfo);

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_RenderPass;
    RenderPassBeginInfo.framebuffer = m_FrameBuffers[vImageIndex];
    RenderPassBeginInfo.renderArea.extent = m_Extent;
    RenderPassBeginInfo.clearValueCount = 1;
    RenderPassBeginInfo.pClearValues = &ClearValue;
    vkCmdBeginRenderPass(m_CommandBuffers[vImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[vImageIndex]);

    vkCmdEndRenderPass(m_CommandBuffers[vImageIndex]);
    ck(vkEndCommandBuffer(m_CommandBuffers[vImageIndex]));

    return m_CommandBuffers[vImageIndex];
}