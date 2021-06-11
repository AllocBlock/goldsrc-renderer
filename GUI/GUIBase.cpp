#include "GUIBase.h"
#include "Common.h"
#include "Log.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>
#include <set>

CGUIBase::CGUIBase(GLFWwindow* vpWindow) : m_pWindow(vpWindow)
{
}

void CGUIBase::init(bool vHasRenderer)
{
    __createInstance();
    if (ENABLE_VALIDATION_LAYERS) __setupDebugMessenger();
    __createSurface();
    __choosePhysicalDevice();
    __createDevice();
    __createDescriptorPool();
    __createSemaphores();
    __createSwapchain();
    __createSwapchainImageViews();

    uint32_t NumImage = static_cast<uint32_t>(m_SwapchainImageViews.size());
    // setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->AddFontFromFileTTF("C:/windows/fonts/simhei.ttf", 13.0f, NULL, IO.Fonts->GetGlyphRangesChineseFull());

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(m_pWindow, true);

    // create renderpass
    VkAttachmentDescription Attachment = {};
    Attachment.format = m_SwapchainImageFormat;
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
    InitInfo.Instance = m_Instance;
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

    __createFramebuffers();

    _initV();
}

void CGUIBase::waitDevice()
{
    ck(vkDeviceWaitIdle(m_Device));
}

void CGUIBase::destroy()
{
    _destroyOtherResourceV();
    __destroySwapchainResources();

    for (size_t i = 0; i < m_MaxFrameInFlight; ++i)
    {
        vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
    }

    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    m_Command.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    if (ENABLE_VALIDATION_LAYERS) __destroyDebugMessenger();
    vkDestroyInstance(m_Instance, nullptr);
}

void CGUIBase::_initV()
{
}

void CGUIBase::_renderV(uint32_t vImageIndex)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(u8"窗口");
    ImGui::Text(u8"默认GUI");
    ImGui::End();
    ImGui::Render();
}

std::vector<VkCommandBuffer> CGUIBase::_getCommandBufferSetV(uint32_t vImageIndex)
{
    return {};
}

void CGUIBase::_createOtherResourceV()
{
}

void CGUIBase::_destroyOtherResourceV()
{
}

void CGUIBase::__createDescriptorPool()
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

void CGUIBase::__createFramebuffers()
{
    uint32_t NumImage = static_cast<uint32_t>(m_SwapchainImageViews.size());
    // create framebuffers
    m_FrameBuffers.clear();
    m_FrameBuffers.resize(NumImage);
    VkFramebufferCreateInfo FramebufferInfo = {};
    FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferInfo.renderPass = m_RenderPass;
    FramebufferInfo.attachmentCount = 1;
    FramebufferInfo.width = m_SwapchainExtent.width;
    FramebufferInfo.height = m_SwapchainExtent.height;
    FramebufferInfo.layers = 1;
    for (uint32_t i = 0; i < NumImage; ++i)
    {
        FramebufferInfo.pAttachments = &m_SwapchainImageViews[i];
        ck(vkCreateFramebuffer(m_Device, &FramebufferInfo, nullptr, &m_FrameBuffers[i]));
    }
}

std::vector<VkCommandBuffer> CGUIBase::requestCommandBufferSet(uint32_t vImageIndex)
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
    RenderPassBeginInfo.framebuffer = m_FrameBuffers[vImageIndex];
    RenderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
    RenderPassBeginInfo.clearValueCount = 1;
    RenderPassBeginInfo.pClearValues = &ClearValue;
    vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);

    vkCmdEndRenderPass(CommandBuffer);
    ck(vkEndCommandBuffer(CommandBuffer));

    std::vector<VkCommandBuffer> CommandBufferSet = _getCommandBufferSetV(vImageIndex);
    CommandBufferSet.emplace_back(CommandBuffer);

    return CommandBufferSet;
}

void CGUIBase::render()
{
    ck(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max()));
    ck(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex]));

    uint32_t ImageIndex;
    VkResult Result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrameIndex], VK_NULL_HANDLE, &ImageIndex);

    _renderV(ImageIndex);

    if (Result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        __recreateSwapchain();
        return;
    }
    else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error(u8"获取交换链图像失败");
    }
    
    std::vector<VkCommandBuffer> CommandBufferSet = requestCommandBufferSet(ImageIndex);

    VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrameIndex] };
    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask = WaitStages;
    SubmitInfo.commandBufferCount = static_cast<uint32_t>(CommandBufferSet.size());
    SubmitInfo.pCommandBuffers = CommandBufferSet.data();

    VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrameIndex] };
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = SignalSemaphores;

    ck(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex]));
    ck(vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, m_InFlightFences[m_CurrentFrameIndex]));

    VkSwapchainKHR SwapChains[] = { m_Swapchain };
    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = SignalSemaphores;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = SwapChains;
    PresentInfo.pImageIndices = &ImageIndex;

    Result = vkQueuePresentKHR(m_PresentQueue, &PresentInfo);
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
    {
        m_FramebufferResized = false;
        __recreateSwapchain();
    }
    else if (Result != VK_SUCCESS)
    {
        throw std::runtime_error(u8"获取交换链图像失败");
    }

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFrameInFlight;
}

void CGUIBase::__createInstance()
{
    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = "GoldSrc Renderer";
    AppInfo.applicationVersion = 1;
    AppInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo InstanceInfo = {};
    InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceInfo.pApplicationInfo = &AppInfo;

    if (ENABLE_VALIDATION_LAYERS && !Common::checkValidationLayerSupport(m_ValidationLayers))
        throw std::runtime_error(u8"不支持所需的验证层");

    if (ENABLE_VALIDATION_LAYERS)
    {
        InstanceInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        InstanceInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    }
    else
    {
        InstanceInfo.enabledLayerCount = 0;
    }

    std::vector<const char*> Extensions = __getRequiredExtensions();
    InstanceInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
    InstanceInfo.ppEnabledExtensionNames = Extensions.data();

    ck(vkCreateInstance(&InstanceInfo, nullptr, &m_Instance));
}

void CGUIBase::__setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerInfo = {};
    DebugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    DebugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    DebugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    DebugMessengerInfo.pfnUserCallback = debugCallback;
    DebugMessengerInfo.pUserData = nullptr;

    auto pCreateDebugFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
    if (pCreateDebugFunc == nullptr)
        throw std::runtime_error(u8"不支持调试函数");
    else
        ck(pCreateDebugFunc(m_Instance, &DebugMessengerInfo, nullptr, &m_DebugMessenger));
}


void CGUIBase::__createSurface()
{
    ck(glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_Surface));
}

void CGUIBase::__choosePhysicalDevice()
{
    uint32_t NumPhysicalDevice = 0;
    std::vector<VkPhysicalDevice> PhysicalDevices;
    ck(vkEnumeratePhysicalDevices(m_Instance, &NumPhysicalDevice, nullptr));
    PhysicalDevices.resize(NumPhysicalDevice);
    ck(vkEnumeratePhysicalDevices(m_Instance, &NumPhysicalDevice, PhysicalDevices.data()));

    for (const auto& PhysicalDevice : PhysicalDevices)
    {
        if (Common::isDeviceSuitable(PhysicalDevice, m_Surface, m_DeviceExtensions))
        {
            m_PhysicalDevice = PhysicalDevice;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error(u8"未找到可用的GPU设备");
    }

    uint32_t ExtensionNum = 0;
    std::vector<VkExtensionProperties> ExtensionProperties;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionNum, nullptr);
    ExtensionProperties.resize(ExtensionNum);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionNum, ExtensionProperties.data());

    for (const VkExtensionProperties& Extension : ExtensionProperties)
    {
        std::cout << "[扩展] " << Extension.extensionName << ", " << Extension.specVersion << std::endl;
    }
}

void CGUIBase::__createDevice()
{
    Common::SQueueFamilyIndices QueueIndices = Common::findQueueFamilies(m_PhysicalDevice, m_Surface);

    std::vector<VkDeviceQueueCreateInfo> QueueInfos;
    std::set<uint32_t> UniqueQueueFamilies = { QueueIndices.GraphicsFamilyIndex.value(), QueueIndices.PresentFamilyIndex.value() };

    float QueuePriority = 1.0f;
    for (uint32_t QueueFamily : UniqueQueueFamilies) {
        VkDeviceQueueCreateInfo QueueInfo = {};
        QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueInfo.queueFamilyIndex = QueueFamily;
        QueueInfo.queueCount = 1;
        QueueInfo.pQueuePriorities = &QueuePriority;
        QueueInfos.emplace_back(QueueInfo);
    }

    VkPhysicalDeviceFeatures RequiredFeatures = {};
    RequiredFeatures.tessellationShader = VK_TRUE;
    RequiredFeatures.geometryShader = VK_TRUE;
    RequiredFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo DeviceInfo = {};
    DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueInfos.size());
    DeviceInfo.pQueueCreateInfos = QueueInfos.data();
    DeviceInfo.pEnabledFeatures = &RequiredFeatures;
    DeviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
    DeviceInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

    if (ENABLE_VALIDATION_LAYERS) {
        DeviceInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        DeviceInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    }
    else {
        DeviceInfo.enabledLayerCount = 0;
    }

    ck(vkCreateDevice(m_PhysicalDevice, &DeviceInfo, nullptr, &m_Device));

    m_GraphicsQueueIndex = QueueIndices.GraphicsFamilyIndex.value();
    m_PresentQueueFamily = QueueIndices.PresentFamilyIndex.value();
    vkGetDeviceQueue(m_Device, QueueIndices.GraphicsFamilyIndex.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, QueueIndices.PresentFamilyIndex.value(), 0, &m_PresentQueue);
}

void CGUIBase::__createSwapchain()
{
    Common::SSwapChainSupportDetails SwapChainSupport = Common::getSwapChainSupport(m_PhysicalDevice, m_Surface);

    VkSurfaceFormatKHR SurfaceFormat = __chooseSwapSurfaceFormat(SwapChainSupport.Formats);
    VkPresentModeKHR PresentMode = __chooseSwapPresentMode(SwapChainSupport.PresentModes);
    VkExtent2D Extent = __chooseSwapExtent(SwapChainSupport.Capabilities);

    uint32_t NumImage = SwapChainSupport.Capabilities.minImageCount + 1;
    if (SwapChainSupport.Capabilities.maxImageCount > 0 &&
        NumImage > SwapChainSupport.Capabilities.maxImageCount)
    {
        NumImage = SwapChainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR SwapChainInfo = {};
    SwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainInfo.surface = m_Surface;
    SwapChainInfo.minImageCount = NumImage;
    SwapChainInfo.imageFormat = SurfaceFormat.format;
    SwapChainInfo.imageColorSpace = SurfaceFormat.colorSpace;
    SwapChainInfo.imageExtent = Extent;
    SwapChainInfo.imageArrayLayers = 1;
    SwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    Common::SQueueFamilyIndices QueueIndices = Common::findQueueFamilies(m_PhysicalDevice, m_Surface);
    uint32_t QueueFamilyIndices[] = { QueueIndices.GraphicsFamilyIndex.value(), QueueIndices.PresentFamilyIndex.value() };

    if (QueueIndices.GraphicsFamilyIndex != QueueIndices.PresentFamilyIndex)
    {
        SwapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapChainInfo.queueFamilyIndexCount = 2;
        SwapChainInfo.pQueueFamilyIndices = QueueFamilyIndices;
    }
    else
    {
        SwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapChainInfo.queueFamilyIndexCount = 0;
        SwapChainInfo.pQueueFamilyIndices = nullptr;
    }
    SwapChainInfo.preTransform = SwapChainSupport.Capabilities.currentTransform;
    SwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapChainInfo.presentMode = PresentMode;
    SwapChainInfo.clipped = VK_TRUE;
    SwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    ck(vkCreateSwapchainKHR(m_Device, &SwapChainInfo, nullptr, &m_Swapchain));

    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &NumImage, nullptr);
    m_SwapchainImages.resize(NumImage);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &NumImage, m_SwapchainImages.data());
    m_SwapchainImageFormat = SurfaceFormat.format;
    m_SwapchainExtent = Extent;
}

void CGUIBase::__createSwapchainImageViews()
{
    m_SwapchainImageViews.resize(m_SwapchainImages.size());

    for (uint32_t i = 0; i < m_SwapchainImageViews.size(); ++i)
    {
        m_SwapchainImageViews[i] = Common::createImageView(m_Device, m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void CGUIBase::__destroySwapchainResources()
{
    for (auto Framebuffer : m_FrameBuffers)
        vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);
    for (auto& ImageView : m_SwapchainImageViews)
        vkDestroyImageView(m_Device, ImageView, nullptr);

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
}

void CGUIBase::__createSemaphores()
{
    m_ImageAvailableSemaphores.resize(m_MaxFrameInFlight);
    m_RenderFinishedSemaphores.resize(m_MaxFrameInFlight);
    m_InFlightFences.resize(m_MaxFrameInFlight);

    VkSemaphoreCreateInfo SemaphoreInfo = {};
    SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo FenceInfo = {};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_MaxFrameInFlight; ++i)
    {
        ck(vkCreateSemaphore(m_Device, &SemaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
        ck(vkCreateSemaphore(m_Device, &SemaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
        ck(vkCreateFence(m_Device, &FenceInfo, nullptr, &m_InFlightFences[i]));
    }
}

void CGUIBase::__recreateSwapchain()
{
    waitDevice();
    __destroySwapchainResources();
    __createSwapchain();
    __createSwapchainImageViews();
    __createFramebuffers();
    _destroyOtherResourceV();
    _createOtherResourceV();
}

VkSurfaceFormatKHR CGUIBase::__chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats)
{
    // 最好的情况，surface没有倾向的格式，可以任意选择
    if (vAvailableFormats.size() == 1 && vAvailableFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // 否则我们看是否有我们所需的
    for (const auto& Format : vAvailableFormats)
    {
        if (Format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return Format;
        }
    }
    // 如果都没有，则需要比较使用哪个格式最好，但一般返回第一个就可以了
    return vAvailableFormats[0];
}

VkPresentModeKHR CGUIBase::__chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes)
{
    VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR; // 一定支持的模式

    for (const auto& PresentMode : vAvailablePresentModes)
    {
        if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR) // triple buffering， 最优先
        {
            BestMode = PresentMode;
            break;
        }
        else if (PresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            BestMode = PresentMode;
        }
    }

    return BestMode;
}

VkExtent2D CGUIBase::__chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities) {
    // swap extent相当于绘制区域，大部分情况下和窗口大小相同

    // currentExtent是当前suface的长宽，如果是(0xFFFFFFFF, 0xFFFFFFFF)说明长宽会有对应的swapchain的extent决定
    if (vCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return vCapabilities.currentExtent;
    }
    else
    {
        int Width, Height;
        glfwGetFramebufferSize(m_pWindow, &Width, &Height);

        VkExtent2D ActualExtent =
        {
            static_cast<uint32_t>(Width),
            static_cast<uint32_t>(Height)
        };
        return ActualExtent;
    }
}

std::vector<const char*> CGUIBase::__getRequiredExtensions()
{
    // 获取GLFW所需的扩展
    uint32_t GlfwExtensionCount = 0;
    const char** GlfwExtensions;
    GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);

    std::vector<const char*> Extensions(GlfwExtensions, GlfwExtensions + GlfwExtensionCount);

    if (ENABLE_VALIDATION_LAYERS) {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return Extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL CGUIBase::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData)
{
    static std::string LastMessage = "";
    if (LastMessage != vpCallbackData->pMessage)
    {
        LastMessage = vpCallbackData->pMessage;
        std::cerr << "[验证层] " << LastMessage << std::endl;
        Common::Log::log(u8"[验证层] " + LastMessage);
    }

    return VK_FALSE;
}

void CGUIBase::__destroyDebugMessenger()
{
    auto pDestroyDebugFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (pDestroyDebugFunc == nullptr)
        throw std::runtime_error(u8"不支持调试函数");
    else
        pDestroyDebugFunc(m_Instance, m_DebugMessenger, nullptr);
}