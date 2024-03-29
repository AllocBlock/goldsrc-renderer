#include "PchVulkan.h"
#include "PhysicalDevice.h"
#include "Vulkan.h"
#include "Log.h"

#include <set>

using namespace vk;

CPhysicalDevice::~CPhysicalDevice()
{
    release(); // this can auto release as no actual handle is created
}

CPhysicalDevice::Ptr CPhysicalDevice::chooseBestDevice(CInstance::CPtr vInstance, CSurface::CPtr vSurface, const std::vector<const char*>& vExtensions)
{
    uint32_t NumPhysicalDevice = 0;
    std::vector<VkPhysicalDevice> PhysicalDevices;
    checkError(vkEnumeratePhysicalDevices(*vInstance, &NumPhysicalDevice, nullptr));
    PhysicalDevices.resize(NumPhysicalDevice);
    checkError(vkEnumeratePhysicalDevices(*vInstance, &NumPhysicalDevice, PhysicalDevices.data()));

    VkPhysicalDevice ChosenDevice = VK_NULL_HANDLE;
    for (const auto& PhysicalDevice : PhysicalDevices)
    {
        if (CPhysicalDevice::__isDeviceSuitable(PhysicalDevice, vSurface, vExtensions))
        {
            ChosenDevice = PhysicalDevice;
            break;
        }
    }

    if (ChosenDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error(u8"未找到可用的GPU设备");
    }

    CPhysicalDevice::Ptr pDevice = make<CPhysicalDevice>();
    // get property, queue family info and swapchain support info
    vkGetPhysicalDeviceProperties(ChosenDevice, &pDevice->m_DeviceProperty);
    pDevice->_set(ChosenDevice);
    pDevice->m_pInstance = vInstance;
    pDevice->m_pSurface = vSurface;
    pDevice->__initBestDepthFormat();
    return pDevice;
}

void CPhysicalDevice::printSupportedExtension() const
{
    uint32_t ExtensionNum = 0;
    std::vector<VkExtensionProperties> ExtensionProperties;
    vkEnumerateDeviceExtensionProperties(get(), nullptr, &ExtensionNum, nullptr);
    ExtensionProperties.resize(ExtensionNum);
    vkEnumerateDeviceExtensionProperties(get(), nullptr, &ExtensionNum, ExtensionProperties.data());

    for (const VkExtensionProperties& Extension : ExtensionProperties)
    {
        Log::log("[扩展] " + std::string(Extension.extensionName) + ", " + std::to_string(Extension.specVersion));
    }
}

CInstance::CPtr CPhysicalDevice::getInstance() const
{
    return m_pInstance;
}

CSurface::CPtr CPhysicalDevice::getSurface() const
{
    return m_pSurface;
}

const VkPhysicalDeviceProperties& CPhysicalDevice::getProperty() const
{
    return m_DeviceProperty;
}

void CPhysicalDevice::release()
{
    _setNull();
    m_pInstance = nullptr;
    m_pSurface = VK_NULL_HANDLE;
    m_DeviceProperty = {};
}

uint32_t CPhysicalDevice::findMemoryTypeIndex(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties) const
{
    _ASSERTE(isValid());
    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(get(), &MemProperties);
    for (uint32_t i = 0; i < MemProperties.memoryTypeCount; ++i)
    {
        if (vTypeFilter & (1 << i) &&
            (MemProperties.memoryTypes[i].propertyFlags & vProperties))
        {
            return i;
        }
    }

    throw std::runtime_error(u8"未找到合适的存储类别");
}

SQueueFamilyIndices CPhysicalDevice::getQueueFamilyInfo() const
{
    _ASSERTE(m_pSurface); 
    return __findQueueFamilies(get(), m_pSurface);
}

SSwapChainSupportDetails CPhysicalDevice::getSwapChainSupportInfo() const
{
    // 需要每次获取都重新查询，因为Surface可能发生变化，可能导致值改变，如Capability
    _ASSERTE(m_pSurface);
    return __getSwapChainSupport(get(), m_pSurface);
}

VkFormat CPhysicalDevice::chooseSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures) const
{
    for (VkFormat Format : vCandidates)
    {
        VkFormatProperties Props = getFormatProperty(Format);

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

    throw std::runtime_error(u8"未找到适配的vulkan格式");
}

VkFormatProperties CPhysicalDevice::getFormatProperty(VkFormat vFormat) const
{
    VkFormatProperties Props;
    vkGetPhysicalDeviceFormatProperties(get(), vFormat, &Props);
    return std::move(Props);
}

VkFormat vk::CPhysicalDevice::getBestDepthFormat() const
{
    return m_BestDepthFormat;
}

bool CPhysicalDevice::__isDeviceSuitable(VkPhysicalDevice vPhysicalDevice, CSurface::CPtr vSurface, const std::vector<const char*>& vDeviceExtensions)
{
    SQueueFamilyIndices QueueIndices = CPhysicalDevice::__findQueueFamilies(vPhysicalDevice, vSurface);
    if (!QueueIndices.isComplete()) return false;

    bool ExtensionsSupported = CPhysicalDevice::__isDeviceExtensionsSupported(vPhysicalDevice, vDeviceExtensions);
    if (!ExtensionsSupported) return false;

    bool SwapChainAdequate = false;
    SSwapChainSupportDetails SwapChainSupport = CPhysicalDevice::__getSwapChainSupport(vPhysicalDevice, vSurface);
    SwapChainAdequate = !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();
    if (!SwapChainAdequate) return false;

    VkPhysicalDeviceFeatures SupportedFeatures;
    vkGetPhysicalDeviceFeatures(vPhysicalDevice, &SupportedFeatures);
    if (!SupportedFeatures.samplerAnisotropy) return false;

    return true;
}

bool CPhysicalDevice::__isDeviceExtensionsSupported(VkPhysicalDevice vPhysicalDevice, const std::vector<const char*>& vDeviceExtensions)
{
    uint32_t NumExtensions;
    checkError(vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &NumExtensions, nullptr));
    std::vector<VkExtensionProperties> AvailableExtensions(NumExtensions);
    checkError(vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &NumExtensions, AvailableExtensions.data()));

    std::set<std::string> RequiredExtensions(vDeviceExtensions.begin(), vDeviceExtensions.end());

    for (const auto& Extension : AvailableExtensions) {
        RequiredExtensions.erase(Extension.extensionName);
    }

    return RequiredExtensions.empty();
}

SQueueFamilyIndices CPhysicalDevice::__findQueueFamilies(VkPhysicalDevice vPhysicalDevice, CSurface::CPtr vSurface)
{
    _ASSERTE(vSurface && vSurface->isValid());

    VkSurfaceKHR Surface = *vSurface;

    uint32_t NumQueueFamily = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &NumQueueFamily, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(NumQueueFamily);
    vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &NumQueueFamily, QueueFamilies.data());

    SQueueFamilyIndices Indices;
    for (uint32_t i = 0; i < NumQueueFamily; ++i)
    {
        if (QueueFamilies[i].queueCount > 0 &&
            QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            Indices.GraphicsFamilyIndex = i;
        }
        VkBool32 PresentSupport = false;
        checkError(vkGetPhysicalDeviceSurfaceSupportKHR(vPhysicalDevice, i, Surface, &PresentSupport));

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

SSwapChainSupportDetails CPhysicalDevice::__getSwapChainSupport(VkPhysicalDevice vPhysicalDevice, CSurface::CPtr vSurface)
{
    _ASSERTE(vSurface && vSurface->isValid());

    VkSurfaceKHR Surface = *vSurface;

    SSwapChainSupportDetails Details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vPhysicalDevice, Surface, &Details.Capabilities);

    uint32_t NumFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, Surface, &NumFormats, nullptr);
    if (NumFormats != 0)
    {
        Details.Formats.resize(NumFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, Surface, &NumFormats, Details.Formats.data());
    }

    uint32_t NumPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, Surface, &NumPresentModes, nullptr);
    if (NumPresentModes != 0)
    {
        Details.PresentModes.resize(NumPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, Surface, &NumPresentModes, Details.PresentModes.data());
    }

    return Details;
}

void CPhysicalDevice::__initBestDepthFormat()
{
    m_BestDepthFormat = chooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}