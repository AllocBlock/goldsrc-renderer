#include "VulkanRenderer.h"
#include "IOLog.h"
#include "Common.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

CVulkanRenderer::CVulkanRenderer(GLFWwindow* vpWindow)
    : m_pWindow(vpWindow),
    m_pCamera(new CCamera)
{
}

CVulkanRenderer::~CVulkanRenderer()
{
    delete m_pImgui;
    __cleanupSwapChain();
    vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    for (size_t i = 0; i < m_TextureImages.size(); ++i)
    {
        vkDestroyImageView(m_Device, m_TextureImageViews[i], nullptr);
        vkDestroyImage(m_Device, m_TextureImages[i], nullptr);
        vkFreeMemory(m_Device, m_TextureImageMemories[i], nullptr);
    }
    
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
    vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
    vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
    vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
    for (size_t i = 0; i < m_MaxFrameInFlight; ++i)
    {
        vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    if (ENABLE_VALIDATION_LAYERS) __destroyDebugMessenger();
    vkDestroyInstance(m_Instance, nullptr);
}

void CVulkanRenderer::init()
{
    __createInstance();
    if (ENABLE_VALIDATION_LAYERS) __setupDebugMessenger();
    __createSurface();
    __choosePhysicalDevice();
    __createDevice();
    __createSwapChain();
    __createImageViews();
    __createRenderPass();
    __createDescriptorSetLayout();
    __createGraphicsPipeline();
    __createCommandPool();
    __createDepthResources();
    __createFramebuffers();
    __createTextureImages();
    __createTextureImageViews();
    __createTextureSampler();
    __createVertexBuffer();
    __createIndexBuffer();
    __createUniformBuffers();
    __createDescriptorPool();
    __createDescriptorSets();
    __createCommandBuffers();
    __createSemaphores();

    SQueueFamilyIndices QueueIndex = __findQueueFamilies(m_PhysicalDevice);
    m_pImgui = new CImguiVullkan(m_Instance, m_PhysicalDevice, m_Device, QueueIndex.GraphicsFamilyIndex.value(), m_GraphicsQueue, m_pWindow, m_SwapchainImageFormat, m_SwapchainExtent, m_SwapchainImageViews);
}

void CVulkanRenderer::render()
{
    m_pImgui->render();

    ck(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max()));
    ck(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrameIndex]));

    uint32_t ImageIndex;
    VkResult Result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrameIndex], VK_NULL_HANDLE, &ImageIndex);

    if (Result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        __recreateSwapChain();
        m_pImgui->updateFramebuffers(m_SwapchainExtent, m_SwapchainImageViews);
        return;
    }
    else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        throw "failed to acquire swap chain image!";
    }

    __updateUniformBuffer(ImageIndex);
    VkCommandBuffer ImguiCommandBuffer = m_pImgui->requestCommandBuffer(ImageIndex);

    VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrameIndex] };
    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    std::vector<VkCommandBuffer> CommandBuffers =
    {
        m_CommandBuffers[ImageIndex],
        ImguiCommandBuffer
    };

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask = WaitStages;
    SubmitInfo.commandBufferCount = static_cast<uint32_t>(CommandBuffers.size());
    SubmitInfo.pCommandBuffers = CommandBuffers.data();

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
        __recreateSwapChain();
        m_pImgui->updateFramebuffers(m_SwapchainExtent, m_SwapchainImageViews);
    }
    else if (Result != VK_SUCCESS)
    {
        throw "failed to acquire swap chain image!";
    }

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFrameInFlight;
}

void CVulkanRenderer::waitDevice()
{
    ck(vkDeviceWaitIdle(m_Device));
}

CCamera* CVulkanRenderer::getCamera()
{
    return m_pCamera;
}

GLFWwindow* CVulkanRenderer::getWindow()
{
    return m_pWindow;
}

void CVulkanRenderer::__createInstance()
{
    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = "GoldSrc Renderer";
    AppInfo.applicationVersion = 1;
    AppInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo InstanceInfo = {};
    InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceInfo.pApplicationInfo = &AppInfo;

    if (ENABLE_VALIDATION_LAYERS && !__checkValidationLayerSupport())
        throw "required validation layers not available";

    if (ENABLE_VALIDATION_LAYERS)
    {
        InstanceInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        InstanceInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    }
    else {
        InstanceInfo.enabledLayerCount = 0;
    }

    std::vector<const char*> Extensions = __getRequiredExtensions();
    InstanceInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
    InstanceInfo.ppEnabledExtensionNames = Extensions.data();

    ck(vkCreateInstance(&InstanceInfo, nullptr, &m_Instance));
}

void CVulkanRenderer::__createSurface()
{
    ck(glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_Surface));
}

void CVulkanRenderer::__choosePhysicalDevice()
{
    uint32_t NumPhysicalDevice = 0;
    std::vector<VkPhysicalDevice> PhysicalDevices;
    ck(vkEnumeratePhysicalDevices(m_Instance, &NumPhysicalDevice, nullptr));
    PhysicalDevices.resize(NumPhysicalDevice);
    ck(vkEnumeratePhysicalDevices(m_Instance, &NumPhysicalDevice, PhysicalDevices.data()));

    for (const auto& PhysicalDevice : PhysicalDevices) {

        if (__isDeviceSuitable(PhysicalDevice)) {
            m_PhysicalDevice = PhysicalDevice;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        throw "failed to find a suitable GPU";
    }
}

void CVulkanRenderer::__createDevice()
{
    SQueueFamilyIndices QueueIndices = __findQueueFamilies(m_PhysicalDevice);

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

    vkGetDeviceQueue(m_Device, QueueIndices.GraphicsFamilyIndex.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, QueueIndices.PresentFamilyIndex.value(), 0, &m_PresentQueue);
}

void CVulkanRenderer::__createSwapChain()
{
    SSwapChainSupportDetails SwapChainSupport = __getSwapChainSupport(m_PhysicalDevice);

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

    SQueueFamilyIndices QueueIndices = __findQueueFamilies(m_PhysicalDevice);
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

void CVulkanRenderer::__createImageViews()
{
    m_SwapchainImageViews.resize(m_SwapchainImages.size());

    for (uint32_t i = 0; i < m_SwapchainImageViews.size(); ++i)
    {
        m_SwapchainImageViews[i] = __createImageView(m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void CVulkanRenderer::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = m_SwapchainImageFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // not present but next render pass

    VkAttachmentDescription DepthAttachment = {};
    DepthAttachment.format = __findDepthFormat();
    DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference DepthAttachmentRef = {};
    DepthAttachmentRef.attachment = 1;
    DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDependency SubpassDependency = {};
    SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependency.dstSubpass = 0;
    SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.srcAccessMask = 0;
    SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount = 1;
    SubpassDesc.pColorAttachments = &ColorAttachmentRef;
    SubpassDesc.pDepthStencilAttachment = &DepthAttachmentRef;

    std::array<VkAttachmentDescription, 2> Attachments = { ColorAttachment, DepthAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
    RenderPassInfo.subpassCount = 1;
    RenderPassInfo.pSubpasses = &SubpassDesc;
    RenderPassInfo.dependencyCount = 1;
    RenderPassInfo.pDependencies = &SubpassDependency;

    ck(vkCreateRenderPass(m_Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CVulkanRenderer::__createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding UboVertBinding = {};
    UboVertBinding.binding = 0;
    UboVertBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboVertBinding.descriptorCount = 1;
    UboVertBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding UboFragBinding = {};
    UboFragBinding.binding = 1;
    UboFragBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboFragBinding.descriptorCount = 1;
    UboFragBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding SamplerBinding = {};
    SamplerBinding.binding = 2;
    SamplerBinding.descriptorCount = 1;
    SamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    SamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    SamplerBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding TextureBinding = {};
    TextureBinding.binding = 3;
    TextureBinding.descriptorCount = m_MaxTextureNum;
    TextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    TextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    TextureBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> Bindings = 
    {
        UboVertBinding,
        UboFragBinding,
        SamplerBinding,
        TextureBinding
    };
    VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
    LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
    LayoutInfo.pBindings = Bindings.data();

    ck(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void CVulkanRenderer::__createGraphicsPipeline()
{
    auto VertShaderCode = __readFile("shader/vert.spv");
    auto FragShaderCode = __readFile("shader/frag.spv");

    VkShaderModule VertShaderModule = __createShaderModule(VertShaderCode);
    VkShaderModule FragShaderModule = __createShaderModule(FragShaderCode);

    VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
    VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertShaderStageInfo.module = VertShaderModule;
    VertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
    FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragShaderStageInfo.module = FragShaderModule;
    FragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

    auto BindingDescription = SPointData::getBindingDescription();
    auto AttributeDescriptions = SPointData::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = 1;
    VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size());
    VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
    VertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
    InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport Viewport = {};
    Viewport.width = static_cast<float>(m_SwapchainExtent.width);
    Viewport.height = static_cast<float>(m_SwapchainExtent.height);
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;

    VkRect2D Scissor = {};
    Scissor.offset = { 0, 0 };
    Scissor.extent = m_SwapchainExtent;

    VkPipelineViewportStateCreateInfo ViewportStateInfo = {};
    ViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateInfo.viewportCount = 1;
    ViewportStateInfo.pViewports = &Viewport;
    ViewportStateInfo.scissorCount = 1;
    ViewportStateInfo.pScissors = &Scissor;

    VkPipelineRasterizationStateCreateInfo RasterizerInfo = {};
    RasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerInfo.depthClampEnable = VK_FALSE;
    RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizerInfo.lineWidth = 1.0f;
    /*RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;*/
    RasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo Multisampling = {};
    Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisampling.sampleShadingEnable = VK_FALSE;
    Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    Multisampling.minSampleShading = 1.0f; // Optional
    Multisampling.pSampleMask = nullptr; // Optional
    Multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    Multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo ColorBlending = {};
    ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlending.logicOpEnable = VK_FALSE;
    ColorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    ColorBlending.attachmentCount = 1;
    ColorBlending.pAttachments = &ColorBlendAttachment;
    ColorBlending.blendConstants[0] = 0.0f; // Optional
    ColorBlending.blendConstants[1] = 0.0f; // Optional
    ColorBlending.blendConstants[2] = 0.0f; // Optional
    ColorBlending.blendConstants[3] = 0.0f; // Optional

    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    PipelineLayoutInfo.pushConstantRangeCount = 1;
    PipelineLayoutInfo.pPushConstantRanges = &PushConstantInfo;

    ck(vkCreatePipelineLayout(m_Device, &PipelineLayoutInfo, nullptr, &m_PipelineLayout));

    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = 2;
    PipelineInfo.pStages = ShaderStages;
    PipelineInfo.pVertexInputState = &VertexInputInfo;
    PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
    PipelineInfo.pViewportState = &ViewportStateInfo;
    PipelineInfo.pRasterizationState = &RasterizerInfo;
    PipelineInfo.pMultisampleState = &Multisampling;
    PipelineInfo.pDepthStencilState = &DepthStencilInfo;
    PipelineInfo.pColorBlendState = &ColorBlending;
    PipelineInfo.pDynamicState = nullptr; // Optional
    PipelineInfo.layout = m_PipelineLayout;
    PipelineInfo.renderPass = m_RenderPass;
    PipelineInfo.subpass = 0;

    ck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_Pipeline));

    vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
}

void CVulkanRenderer::__createCommandPool()
{
    SQueueFamilyIndices QueueFamilyIndices = __findQueueFamilies(m_PhysicalDevice);

    VkCommandPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.queueFamilyIndex = QueueFamilyIndices.GraphicsFamilyIndex.value();

    ck(vkCreateCommandPool(m_Device, &PoolInfo, nullptr, &m_CommandPool));
}

void CVulkanRenderer::__createDepthResources()
{
    VkFormat DepthFormat = __findDepthFormat();
    __createImage(m_SwapchainExtent.width, m_SwapchainExtent.height, DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = __createImageView(m_DepthImage, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    __transitionImageLayout(m_DepthImage, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void CVulkanRenderer::__createFramebuffers()
{
    m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());
    for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
    {
        std::array<VkImageView, 2> Attachments =
        {
            m_SwapchainImageViews[i],
            m_DepthImageView
        };

        VkFramebufferCreateInfo FramebufferInfo = {};
        FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass = m_RenderPass;
        FramebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
        FramebufferInfo.pAttachments = Attachments.data();
        FramebufferInfo.width = m_SwapchainExtent.width;
        FramebufferInfo.height = m_SwapchainExtent.height;
        FramebufferInfo.layers = 1;

        ck(vkCreateFramebuffer(m_Device, &FramebufferInfo, nullptr, &m_SwapchainFramebuffers[i]));
    }
}

void CVulkanRenderer::__createTextureImages()
{
    size_t NumTextures = __getActualTextureNum();
    m_TextureImages.resize(NumTextures);
    m_TextureImageMemories.resize(NumTextures);
    for (size_t i = 0; i < NumTextures; ++i)
    {
        const CIOImage& Image = m_TextureImageData[i];
        int TexWidth = Image.getImageWidth();
        int TexHeight = Image.getImageHeight();
        const void* pPixelData = Image.getData();

        VkDeviceSize DataSize = static_cast<uint64_t>(4) * TexWidth * TexHeight;
        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;
        __createBuffer(DataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

        void* pDevData;
        ck(vkMapMemory(m_Device, StagingBufferMemory, 0, DataSize, 0, &pDevData));
        memcpy(pDevData, pPixelData, static_cast<size_t>(DataSize));
        vkUnmapMemory(m_Device, StagingBufferMemory);

        __createImage(TexWidth, TexHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImages[i], m_TextureImageMemories[i]);
        __transitionImageLayout(m_TextureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        __copyBufferToImage(StagingBuffer, m_TextureImages[i], TexWidth, TexHeight);
        __transitionImageLayout(m_TextureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
        vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
    }
}

void CVulkanRenderer::__createTextureImageViews()
{
    size_t NumTextures = __getActualTextureNum();
    m_TextureImageViews.resize(NumTextures);
    for (size_t i = 0; i < NumTextures; ++i)
        m_TextureImageViews[i] = __createImageView(m_TextureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CVulkanRenderer::__createTextureSampler()
{
    VkPhysicalDeviceProperties Properties = {};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &Properties);

    VkSamplerCreateInfo SamplerInfo = {};
    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerInfo.magFilter = VK_FILTER_LINEAR;
    SamplerInfo.minFilter = VK_FILTER_LINEAR;
    SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.anisotropyEnable = VK_TRUE;
    SamplerInfo.maxAnisotropy = Properties.limits.maxSamplerAnisotropy;
    SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerInfo.compareEnable = VK_FALSE;
    SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.mipLodBias = 0.0f;
    SamplerInfo.minLod = 0.0f;
    SamplerInfo.maxLod = 0.0f;

    ck(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler));
}

void CVulkanRenderer::__createVertexBuffer()
{
    if (m_SceneObjects.empty()) throw "no object to render";

    size_t NumVertex = 0;
    for (const S3DObject& Object : m_SceneObjects)
        NumVertex += Object.Vertices.size();

    VkDeviceSize BufferSize = sizeof(SPointData) * NumVertex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    size_t Offset = 0;
    for (const S3DObject& Object : m_SceneObjects)
    {
        std::vector<SPointData> PointData = Object.getPointData();
        size_t SubBufferSize = sizeof(SPointData) * Object.Vertices.size();
        memcpy(reinterpret_cast<char*>(pData)+ Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

    __copyBuffer(StagingBuffer, m_VertexBuffer, BufferSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__createIndexBuffer()
{
    if (m_SceneObjects.empty()) throw "no object to render";

    size_t NumVertex = 0;
    for (const S3DObject& Object : m_SceneObjects)
        NumVertex += Object.Indices.size();

    VkDeviceSize BufferSize = sizeof(uint32_t) * NumVertex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    size_t Offset = 0;
    for (const S3DObject& Object : m_SceneObjects)
    {
        size_t IndexOffset = Offset / sizeof(uint32_t);
        std::vector<uint32_t> Indices = Object.Indices;
        for (uint32_t& Index : Indices)
            Index += IndexOffset;
        size_t SubBufferSize = sizeof(uint32_t) * Indices.size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, Indices.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

    __copyBuffer(StagingBuffer, m_IndexBuffer, BufferSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__createUniformBuffers()
{
    VkDeviceSize BufferSize = sizeof(SUniformBufferObjectVert);
    size_t NumSwapchainImage = m_SwapchainImages.size();
    m_VertUniformBuffers.resize(NumSwapchainImage);
    m_VertUniformBufferMemories.resize(NumSwapchainImage);
    m_FragUniformBuffers.resize(NumSwapchainImage);
    m_FragUniformBufferMemories.resize(NumSwapchainImage);

    for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
    {
        __createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBuffers[i], m_VertUniformBufferMemories[i]);
        __createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBuffers[i], m_FragUniformBufferMemories[i]);
    }
}

void CVulkanRenderer::__createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> PoolSizes =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_SwapchainImages.size()) },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_SwapchainImages.size()) },
        { VK_DESCRIPTOR_TYPE_SAMPLER, static_cast<uint32_t>(m_SwapchainImages.size()) },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(m_SwapchainImages.size() * m_MaxTextureNum) }
    };

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
    PoolInfo.pPoolSizes = PoolSizes.data();
    PoolInfo.maxSets = static_cast<uint32_t>(m_SwapchainImages.size());

    ck(vkCreateDescriptorPool(m_Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CVulkanRenderer::__createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> Layouts(m_SwapchainImages.size(), m_DescriptorSetLayout);

    VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
    DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool = m_DescriptorPool;
    DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapchainImages.size());
    DescSetAllocInfo.pSetLayouts = Layouts.data();

    m_DescriptorSets.resize(m_SwapchainImages.size());
    ck(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_DescriptorSets.data()));

    __updateDescriptorSets();
}

void CVulkanRenderer::__updateDescriptorSets()
{
    for (size_t i = 0; i < m_DescriptorSets.size(); ++i)
    {
        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBuffers[i];
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUniformBufferObjectVert);

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBuffers[i];
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUniformBufferObjectFrag);

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = VK_NULL_HANDLE;
        SamplerInfo.sampler = m_TextureSampler;

        const size_t NumTexture = __getActualTextureNum();
        std::vector<VkDescriptorImageInfo> ImageInfos(m_MaxTextureNum);
        for (size_t i = 0; i < m_MaxTextureNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                ImageInfos[i] = ImageInfos[0];
            }
            else
            {
                ImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                ImageInfos[i].imageView = m_TextureImageViews[i];
                ImageInfos[i].sampler = VK_NULL_HANDLE;
            }
        }
        
        std::array<VkWriteDescriptorSet, 4> DescriptorWrites = {};
        DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[0].dstSet = m_DescriptorSets[i];
        DescriptorWrites[0].dstBinding = 0;
        DescriptorWrites[0].dstArrayElement = 0;
        DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorWrites[0].descriptorCount = 1;
        DescriptorWrites[0].pBufferInfo = &VertBufferInfo;

        DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[1].dstSet = m_DescriptorSets[i];
        DescriptorWrites[1].dstBinding = 1;
        DescriptorWrites[1].dstArrayElement = 0;
        DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorWrites[1].descriptorCount = 1;
        DescriptorWrites[1].pBufferInfo = &FragBufferInfo;

        DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[2].dstSet = m_DescriptorSets[i];
        DescriptorWrites[2].dstBinding = 2;
        DescriptorWrites[2].dstArrayElement = 0;
        DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        DescriptorWrites[2].descriptorCount = 1;
        DescriptorWrites[2].pImageInfo = &SamplerInfo;

        DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[3].dstSet = m_DescriptorSets[i];
        DescriptorWrites[3].dstBinding = 3;
        DescriptorWrites[3].dstArrayElement = 0;
        DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        DescriptorWrites[3].descriptorCount = static_cast<uint32_t>(ImageInfos.size());
        DescriptorWrites[3].pImageInfo = ImageInfos.data();

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
    }
}

void CVulkanRenderer::__createCommandBuffers()
{
    m_CommandBuffers.resize(m_SwapchainFramebuffers.size());

    VkCommandBufferAllocateInfo CommandBufferAllocInfo = {};
    CommandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocInfo.commandPool = m_CommandPool;
    CommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    ck(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocInfo, m_CommandBuffers.data()));

    for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
    {
        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        ck(vkBeginCommandBuffer(m_CommandBuffers[i], &CommandBufferBeginInfo));

        std::array<VkClearValue, 2> ClearValues = {};
        ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo RenderPassBeginInfo = {};
        RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass = m_RenderPass;
        RenderPassBeginInfo.framebuffer = m_SwapchainFramebuffers[i];
        RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        RenderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
        RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
        RenderPassBeginInfo.pClearValues = ClearValues.data();

        vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

        VkBuffer VertexBuffers[] = { m_VertexBuffer };
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, VertexBuffers, Offsets);
        vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr);

        SPushConstant PushConstant = {};

        size_t IndexOffset = 0;
        for (const S3DObject& Object : m_SceneObjects)
        {
            size_t NumIndex = Object.Indices.size();
            if (NumIndex == 0) continue;
            PushConstant.TexIndex = Object.TexIndex;
            vkCmdPushConstants(m_CommandBuffers[i], m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstant), &PushConstant);
            vkCmdDrawIndexed(m_CommandBuffers[i], NumIndex, 1, IndexOffset, 0, 0);
            IndexOffset += NumIndex;
        }

        vkCmdEndRenderPass(m_CommandBuffers[i]);
        ck(vkEndCommandBuffer(m_CommandBuffers[i]));
    }
}

void CVulkanRenderer::__createSemaphores()
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

std::vector<char> CVulkanRenderer::__readFile(std::string vFileName)
{
    std::ifstream File(vFileName, std::ios::ate | std::ios::binary);

    if (!File.is_open()) {
        throw "failed to open file " + vFileName;
    }
    size_t FileSize = static_cast<size_t>(File.tellg());
    std::vector<char> Buffer(FileSize);
    File.seekg(0);
    File.read(Buffer.data(), FileSize);
    File.close();

    return Buffer;
}


bool CVulkanRenderer::__checkValidationLayerSupport()
{
    uint32_t LayerCount;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
    std::vector<VkLayerProperties> AvailableLayers(LayerCount);
    vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

    for (const char* RequiredLayerName : m_ValidationLayers) {
        bool LayerFound = false;
        for (const auto& Layer : AvailableLayers) {
            if (strcmp(RequiredLayerName, Layer.layerName) == 0) {
                LayerFound = true;
                break;
            }
        }
        if (!LayerFound) {
            return false;
        }
    }
    return true;
}

std::vector<const char*> CVulkanRenderer::__getRequiredExtensions()
{
    // 获取GLFW所需的扩展
    uint32_t GlfwExtensionCount = 0;
    const char** GlfwExtensions;
    GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);

    std::vector<const char*> Extensions(GlfwExtensions, GlfwExtensions + GlfwExtensionCount);

    if (ENABLE_VALIDATION_LAYERS) {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // TODO: 检查是否是指所有的扩展（extension）

    return Extensions;
}

bool CVulkanRenderer::__isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice)
{
    SQueueFamilyIndices QueueIndices = __findQueueFamilies(vPhysicalDevice);
    if (!QueueIndices.isComplete()) return false;

    bool ExtensionsSupported = __checkDeviceExtensionSupport(vPhysicalDevice);
    if (!ExtensionsSupported) return false;

    bool SwapChainAdequate = false;
    SSwapChainSupportDetails SwapChainSupport = __getSwapChainSupport(vPhysicalDevice);
    SwapChainAdequate = !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();
    if (!SwapChainAdequate) return false;

    VkPhysicalDeviceFeatures SupportedFeatures;
    vkGetPhysicalDeviceFeatures(vPhysicalDevice, &SupportedFeatures);
    if (!SupportedFeatures.samplerAnisotropy) return false;

    return true;
}

SQueueFamilyIndices CVulkanRenderer::__findQueueFamilies(const VkPhysicalDevice& vPhysicalDevice)
{
    uint32_t NumQueueFamily = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &NumQueueFamily, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(NumQueueFamily);
    vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &NumQueueFamily, QueueFamilies.data());

    SQueueFamilyIndices Indices;
    for (size_t i = 0; i < NumQueueFamily; ++i)
    {
        if (QueueFamilies[i].queueCount > 0 &&
            QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            Indices.GraphicsFamilyIndex = i;
        }
        VkBool32 PresentSupport = false;
        ck(vkGetPhysicalDeviceSurfaceSupportKHR(vPhysicalDevice, i, m_Surface, &PresentSupport));

        if (QueueFamilies[i].queueCount > 0 && PresentSupport)
        {
            Indices.PresentFamilyIndex = i;
        }
        if (Indices.isComplete())
        {
            break;
        }
    }
    return Indices;
}

bool CVulkanRenderer::__checkDeviceExtensionSupport(const VkPhysicalDevice& vPhysicalDevice)
{
    uint32_t NumExtensions;
    ck(vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &NumExtensions, nullptr));
    std::vector<VkExtensionProperties> AvailableExtensions(NumExtensions);
    ck(vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &NumExtensions, AvailableExtensions.data()));

    std::set<std::string> RequiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

    for (const auto& Extension : AvailableExtensions) {
        RequiredExtensions.erase(Extension.extensionName);
    }

    return RequiredExtensions.empty();
}

SSwapChainSupportDetails CVulkanRenderer::__getSwapChainSupport(const VkPhysicalDevice& vPhysicalDevice)
{
    SSwapChainSupportDetails Details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vPhysicalDevice, m_Surface, &Details.Capabilities);

    uint32_t NumFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, m_Surface, &NumFormats, nullptr);
    if (NumFormats != 0)
    {
        Details.Formats.resize(NumFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, m_Surface, &NumFormats, Details.Formats.data());
    }

    uint32_t NumPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, m_Surface, &NumPresentModes, nullptr);
    if (NumPresentModes != 0) {
        Details.PresentModes.resize(NumPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, m_Surface, &NumPresentModes, Details.PresentModes.data());
    }

    return Details;
}

VkSurfaceFormatKHR CVulkanRenderer::__chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats)
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

VkPresentModeKHR CVulkanRenderer::__chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes)
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

VkExtent2D CVulkanRenderer::__chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities) {
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

VkImageView CVulkanRenderer::__createImageView(VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags)
{
    VkImageViewCreateInfo ImageViewInfo = {};
    ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewInfo.image = vImage;
    ImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewInfo.format = vFormat;
    ImageViewInfo.subresourceRange.aspectMask = vAspectFlags;
    ImageViewInfo.subresourceRange.baseMipLevel = 0;
    ImageViewInfo.subresourceRange.levelCount = 1;
    ImageViewInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewInfo.subresourceRange.layerCount = 1;

    VkImageView ImageView;
    ck(vkCreateImageView(m_Device, &ImageViewInfo, nullptr, &ImageView));

    return ImageView;
}

VkFormat CVulkanRenderer::__findDepthFormat()
{
    return __findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat CVulkanRenderer::__findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures)
{
    for (VkFormat Format : vCandidates)
    {
        VkFormatProperties Props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, Format, &Props);

        if (vTiling == VK_IMAGE_TILING_LINEAR && 
            (Props.linearTilingFeatures & vFeatures) == vFeatures)
        {
            return Format;
        }
        else if (vTiling == VK_IMAGE_TILING_OPTIMAL && 
            (Props.optimalTilingFeatures & vFeatures) == vFeatures)
        {
            return Format;
        }
    }

    throw "failed to find supported format";
}

VkShaderModule CVulkanRenderer::__createShaderModule(const std::vector<char>& vShaderCode)
{
    VkShaderModuleCreateInfo ShaderModuleInfo = {};
    ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleInfo.codeSize = vShaderCode.size();
    ShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());

    VkShaderModule ShaderModule;
    ck(vkCreateShaderModule(m_Device, &ShaderModuleInfo, nullptr, &ShaderModule));
    return ShaderModule;
}

void CVulkanRenderer::__createImage(uint32_t vWidth, uint32_t vHeight, VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory)
{
    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = vWidth;
    ImageInfo.extent.height = vHeight;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = vFormat;
    ImageInfo.tiling = vTiling;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = vUsage;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ck(vkCreateImage(m_Device, &ImageInfo, nullptr, &voImage));

    VkMemoryRequirements MemRequirements;
    vkGetImageMemoryRequirements(m_Device, voImage, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = __findMemoryType(MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(m_Device, &AllocInfo, nullptr, &voImageMemory));

    ck(vkBindImageMemory(m_Device, voImage, voImageMemory, 0));
}

uint32_t CVulkanRenderer::__findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties)
{
    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &MemProperties);
    for (uint32_t i = 0; i < MemProperties.memoryTypeCount; ++i)
    {
        if (vTypeFilter & (1 << i) && 
            (MemProperties.memoryTypes[i].propertyFlags & vProperties))
        {
            return i;
        }
    }

    throw "failed to find suitable memory type!";
}

void CVulkanRenderer::__transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout) {
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkImageMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = vOldLayout;
    Barrier.newLayout = vNewLayout;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = vImage;

    if (vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (__hasStencilComponent(vFormat))
        {
            Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags SrcStage;
    VkPipelineStageFlags DestStage;

    if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
        && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw "unsupported layout transition!";
    }

    vkCmdPipelineBarrier(
        CommandBuffer,
        SrcStage, DestStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &Barrier
    );

    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

bool CVulkanRenderer::__hasStencilComponent(VkFormat vFormat) {
    return vFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vFormat == VK_FORMAT_D24_UNORM_S8_UINT;
}

void CVulkanRenderer::__createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory)
{
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = vSize;
    BufferInfo.usage = vUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ck(vkCreateBuffer(m_Device, &BufferInfo, nullptr, &voBuffer));

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements(m_Device, voBuffer, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = __findMemoryType(MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(m_Device, &AllocInfo, nullptr, &voBufferMemory));

    ck(vkBindBufferMemory(m_Device, voBuffer, voBufferMemory, 0));
}

void CVulkanRenderer::__copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize)
{
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkBufferCopy CopyRegion = {};
    CopyRegion.size = vSize;
    vkCmdCopyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, 1, &CopyRegion);

    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

void CVulkanRenderer::__copyBufferToImage(VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight)
{
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0;
    Region.bufferImageHeight = 0;

    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = 1;

    Region.imageOffset = { 0, 0, 0 };
    Region.imageExtent = { static_cast<uint32_t>(vWidth), static_cast<uint32_t>(vHeight), 1 };

    vkCmdCopyBufferToImage(CommandBuffer, vBuffer, vImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
    
    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

size_t CVulkanRenderer::__getActualTextureNum()
{
    size_t NumTexture = m_TextureImageData.size();
    if (NumTexture > m_MaxTextureNum)
    {
        GlobalLogger::logStream() << "Warning: Texture Num = (" << std::to_string(NumTexture) << ") is greater than limit (" << std::to_string(m_MaxTextureNum) << "), overflow textures are ignored" << std::endl;
        NumTexture = m_MaxTextureNum;
    }
    return NumTexture;
}

void CVulkanRenderer::__cleanupSwapChain()
{
    vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    vkDestroyImage(m_Device, m_DepthImage, nullptr);
    vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);
    for (auto& Framebuffer : m_SwapchainFramebuffers)
        vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    for (auto& ImageView : m_SwapchainImageViews)
        vkDestroyImageView(m_Device, ImageView, nullptr);

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
    {
        vkDestroyBuffer(m_Device, m_VertUniformBuffers[i], nullptr);
        vkFreeMemory(m_Device, m_VertUniformBufferMemories[i], nullptr);
        vkDestroyBuffer(m_Device, m_FragUniformBuffers[i], nullptr);
        vkFreeMemory(m_Device, m_FragUniformBufferMemories[i], nullptr);
    }
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
}

void CVulkanRenderer::__recreateSwapChain()
{
    vkDeviceWaitIdle(m_Device);
    __cleanupSwapChain();
    __createSwapChain();
    __createImageViews();
    __createRenderPass();
    __createGraphicsPipeline();
    __createDepthResources();
    __createFramebuffers();
    __createUniformBuffers();
    __createDescriptorPool();
    __createDescriptorSets();
    __createCommandBuffers();
}

void CVulkanRenderer::__updateUniformBuffer(uint32_t vImageIndex)
{
    static auto StartTime = std::chrono::high_resolution_clock::now();

    auto CurrentTime = std::chrono::high_resolution_clock::now();
    float DeltaTime = std::chrono::duration<float, std::chrono::seconds::period>(CurrentTime - StartTime).count();

    float Aspect = 1.0;
    if (m_SwapchainExtent.height > 0 && m_SwapchainExtent.width > 0)
        Aspect = static_cast<float>(m_SwapchainExtent.width) / m_SwapchainExtent.height;
    m_pCamera->setAspect(Aspect);
    SUniformBufferObjectVert UBO = {};
    //UBO.Model = glm::rotate(glm::mat4(1.0), DeltaTime * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    UBO.Model = glm::mat4(1.0f);
    UBO.View = m_pCamera->getViewMat();
    UBO.Proj = m_pCamera->getProjMat();

    void* pData;
    ck(vkMapMemory(m_Device, m_VertUniformBufferMemories[vImageIndex], 0, sizeof(UBO), 0, &pData));
    memcpy(pData, &UBO, sizeof(UBO));
    vkUnmapMemory(m_Device, m_VertUniformBufferMemories[vImageIndex]);

    SUniformBufferObjectFrag UBOFrag = {};
    UBOFrag.Eye = m_pCamera->getPos();

    ck(vkMapMemory(m_Device, m_FragUniformBufferMemories[vImageIndex], 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_FragUniformBufferMemories[vImageIndex]);
}

void CVulkanRenderer::__setupDebugMessenger()
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
        throw "debug function not supported";
    else
        ck(pCreateDebugFunc(m_Instance, &DebugMessengerInfo, nullptr, &m_DebugMessenger));
}

void CVulkanRenderer::__destroyDebugMessenger()
{
    auto pDestroyDebugFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (pDestroyDebugFunc == nullptr)
        throw "debug function not supported";
    else
        pDestroyDebugFunc(m_Instance, m_DebugMessenger, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL CVulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData)
{
    static std::string LastMessage = "";
    if (LastMessage != vpCallbackData->pMessage)
    {
        LastMessage = vpCallbackData->pMessage;
        std::cerr << "[Validation Layer] " << vpCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}