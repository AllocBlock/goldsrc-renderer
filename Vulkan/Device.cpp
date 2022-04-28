#include "Device.h"
#include "Vulkan.h"
#include <set>

using namespace vk;

void CDevice::create(VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface, const std::vector<const char*>& vExtensionSet, const std::vector<const char*>& vValidationLayerSet)
{
    destroy();

    Vulkan::SQueueFamilyIndices QueueIndices = Vulkan::findQueueFamilies(vPhysicalDevice, vSurface);

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
    DeviceInfo.enabledExtensionCount = static_cast<uint32_t>(vExtensionSet.size());
    DeviceInfo.ppEnabledExtensionNames = vExtensionSet.data();

    DeviceInfo.enabledLayerCount = static_cast<uint32_t>(vValidationLayerSet.size());
    if (!vValidationLayerSet.empty()) DeviceInfo.ppEnabledLayerNames = vValidationLayerSet.data();

    Vulkan::checkError(vkCreateDevice(vPhysicalDevice, &DeviceInfo, nullptr, &m_Handle));

    m_GraphicsQueueIndex = QueueIndices.GraphicsFamilyIndex.value();
    m_PresentQueueIndex = QueueIndices.PresentFamilyIndex.value();
    vkGetDeviceQueue(m_Handle, QueueIndices.GraphicsFamilyIndex.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Handle, QueueIndices.PresentFamilyIndex.value(), 0, &m_PresentQueue);
}

void CDevice::destroy()
{
    if (m_Handle) vkDestroyDevice(m_Handle, nullptr);
    m_Handle = VK_NULL_HANDLE;
     
    m_GraphicsQueueIndex = m_PresentQueueIndex = 0;
    m_GraphicsQueue = m_PresentQueue = VK_NULL_HANDLE;
}

void CDevice::waitUntilIdle()
{
    Vulkan::checkError(vkDeviceWaitIdle(m_Handle));
}

uint32_t CDevice::getGraphicsQueueIndex()
{
    return m_GraphicsQueueIndex;
}

uint32_t CDevice::getPresentQueueIndex()
{
    return m_PresentQueueIndex;
}

VkQueue CDevice::getGraphicsQueue()
{
    return m_GraphicsQueue;
}

VkQueue CDevice::getPresentQueue()
{
    return m_PresentQueue;
}