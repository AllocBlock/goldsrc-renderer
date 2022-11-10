#include "PchVulkan.h"
#include "Device.h"
#include "Vulkan.h"
#include <set>

using namespace vk;

void CDevice::create(CPhysicalDevice::CPtr vPhysicalDevice, const std::vector<const char*>& vExtensionSet, const std::vector<const char*>& vValidationLayerSet)
{
    destroy();

    m_pPhysicalDevice = vPhysicalDevice;

    const vk::SQueueFamilyIndices& QueueIndices = vPhysicalDevice->getQueueFamilyInfo();

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

    vk::checkError(vkCreateDevice(*vPhysicalDevice, &DeviceInfo, nullptr, _getPtr()));

    m_GraphicsQueueIndex = QueueIndices.GraphicsFamilyIndex.value();
    m_PresentQueueIndex = QueueIndices.PresentFamilyIndex.value();
    m_GraphicsQueue = getQueue(m_GraphicsQueueIndex);
    m_PresentQueue = getQueue(m_PresentQueueIndex);
}

void CDevice::destroy()
{
    if (get()) vkDestroyDevice(get(), nullptr);
    _setNull();
     
    m_GraphicsQueueIndex = m_PresentQueueIndex = 0;
    m_GraphicsQueue = m_PresentQueue = VK_NULL_HANDLE;

    m_pPhysicalDevice = nullptr;
}

void CDevice::waitUntilIdle() const
{
    vk::checkError(vkDeviceWaitIdle(get()));
}

CPhysicalDevice::CPtr CDevice::getPhysicalDevice() const
{
    return m_pPhysicalDevice;
}

uint32_t CDevice::getGraphicsQueueIndex() const
{
    return m_GraphicsQueueIndex;
}

uint32_t CDevice::getPresentQueueIndex() const
{
    return m_PresentQueueIndex;
}

VkQueue CDevice::getGraphicsQueue() const
{
    return m_GraphicsQueue;
}

VkQueue CDevice::getPresentQueue() const
{
    return m_PresentQueue;
}

VkQueue CDevice::getQueue(uint32_t vFamilyIndex) const
{
    VkQueue Queue;
    vkGetDeviceQueue(get(), vFamilyIndex, 0, &Queue);
    return Queue;
}

VkShaderModule CDevice::createShaderModule(const std::vector<uint8_t>& vShaderCode) const
{
    _ASSERTE(isValid());

    VkShaderModuleCreateInfo ShaderModuleInfo = {};
    ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleInfo.codeSize = vShaderCode.size();
    ShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());

    VkShaderModule ShaderModule;
    checkError(vkCreateShaderModule(get(), &ShaderModuleInfo, nullptr, &ShaderModule));
    return ShaderModule;
}

void CDevice::destroyShaderModule(VkShaderModule vModule) const
{
    _ASSERTE(isValid());
    vkDestroyShaderModule(get(), vModule, nullptr);
}

VkSemaphore CDevice::createSemaphore() const
{
    VkSemaphoreCreateInfo SemaphoreInfo = {};
    SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore Semaphore;
    vk::checkError(vkCreateSemaphore(get(), &SemaphoreInfo, nullptr, &Semaphore));
    return Semaphore;
}

void CDevice::destroySemaphore(VkSemaphore vSemaphore) const
{
    vkDestroySemaphore(get(), vSemaphore, nullptr);
}