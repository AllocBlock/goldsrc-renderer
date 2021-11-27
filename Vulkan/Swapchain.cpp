#include "Swapchain.h"
#include "Vulkan.h"

using namespace vk;

void CSwapchain::create(VkDevice vDevice, VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface, GLFWwindow* vWindow)
{
    destroy();

    m_Device = vDevice;

    Vulkan::SSwapChainSupportDetails SwapChainSupport = Vulkan::getSwapChainSupport(vPhysicalDevice, vSurface);
    VkSurfaceFormatKHR SurfaceFormat = __chooseSwapSurfaceFormat(SwapChainSupport.Formats);
    VkPresentModeKHR PresentMode = __chooseSwapPresentMode(SwapChainSupport.PresentModes);
    VkExtent2D Extent = __chooseSwapExtent(vWindow, SwapChainSupport.Capabilities);

    uint32_t NumImage = SwapChainSupport.Capabilities.minImageCount + 1;
    if (SwapChainSupport.Capabilities.maxImageCount > 0 &&
        NumImage > SwapChainSupport.Capabilities.maxImageCount)
    {
        NumImage = SwapChainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR SwapChainInfo = {};
    SwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainInfo.surface = vSurface;
    SwapChainInfo.minImageCount = NumImage;
    SwapChainInfo.imageFormat = SurfaceFormat.format;
    SwapChainInfo.imageColorSpace = SurfaceFormat.colorSpace;
    SwapChainInfo.imageExtent = Extent;
    SwapChainInfo.imageArrayLayers = 1;
    SwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    Vulkan::SQueueFamilyIndices QueueIndices = Vulkan::findQueueFamilies(vPhysicalDevice, vSurface);
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

    Vulkan::checkError(vkCreateSwapchainKHR(vDevice, &SwapChainInfo, nullptr, &m_Handle));

    vkGetSwapchainImagesKHR(vDevice, m_Handle, &NumImage, nullptr);
    m_ImageSet.resize(NumImage);
    vkGetSwapchainImagesKHR(vDevice, m_Handle, &NumImage, m_ImageSet.data());
    m_ImageFormat = SurfaceFormat.format;
    m_Extent = Extent;

    __createImageViews();
}

void CSwapchain::destroy()
{
    if (m_Handle) vkDestroySwapchainKHR(m_Device, m_Handle, nullptr);
    m_Handle = VK_NULL_HANDLE;

    __destroyImageViews();
    m_Device = VK_NULL_HANDLE;
    m_ImageSet.clear();
    m_ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
    m_Extent = { 0, 0 };
}


VkExtent2D CSwapchain::getExtent()
{
    return m_Extent;
}

VkFormat CSwapchain::getImageFormat()
{
    return m_ImageFormat;
}

const std::vector<VkImageView>& CSwapchain::getImageViews()
{
    return m_ImageViewSet;
}

VkSurfaceFormatKHR CSwapchain::__chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats)
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

VkPresentModeKHR CSwapchain::__chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes)
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

VkExtent2D CSwapchain::__chooseSwapExtent(GLFWwindow* vWindow, const VkSurfaceCapabilitiesKHR& vCapabilities) {
    // swap extent相当于绘制区域，大部分情况下和窗口大小相同

    // currentExtent是当前suface的长宽，如果是(0xFFFFFFFF, 0xFFFFFFFF)说明长宽会有对应的swapchain的extent决定
    if (vCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return vCapabilities.currentExtent;
    }
    else
    {
        int Width, Height;
        glfwGetFramebufferSize(vWindow, &Width, &Height);

        VkExtent2D ActualExtent =
        {
            static_cast<uint32_t>(Width),
            static_cast<uint32_t>(Height)
        };
        return ActualExtent;
    }
}

void CSwapchain::__createImageViews()
{
    m_ImageViewSet.resize(m_ImageSet.size());

    for (uint32_t i = 0; i < m_ImageViewSet.size(); ++i)
    {
        m_ImageViewSet[i] = Vulkan::createImageView(m_Device, m_ImageSet[i], m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void CSwapchain::__destroyImageViews()
{
    for (auto& ImageView : m_ImageViewSet)
        vkDestroyImageView(m_Device, ImageView, nullptr);
    m_ImageViewSet.clear();
}
