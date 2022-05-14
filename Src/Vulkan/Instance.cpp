#include "PchVulkan.h"
#include "Instance.h"
#include "Vulkan.h"

using namespace vk;

void CInstance::create(std::string vName, const std::vector<const char*>& vValidationLayers, const std::vector<const char*>& vExtensions)
{
    destroy();

    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = vName.c_str();
    AppInfo.applicationVersion = 1;
    AppInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo InstanceInfo = {};
    InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceInfo.pApplicationInfo = &AppInfo;

    if (!vk::isValidationLayersSupported(vValidationLayers))
        throw std::runtime_error(u8"不支持所需的验证层");

    InstanceInfo.enabledLayerCount = static_cast<uint32_t>(vValidationLayers.size());
    if (!vValidationLayers.empty())
        InstanceInfo.ppEnabledLayerNames = vValidationLayers.data();

    InstanceInfo.enabledExtensionCount = static_cast<uint32_t>(vExtensions.size());
    if (!vExtensions.empty())
        InstanceInfo.ppEnabledExtensionNames = vExtensions.data();

    vk::checkError(vkCreateInstance(&InstanceInfo, nullptr, _getPtr()));
}

void CInstance::destroy()
{
    if (get()) vkDestroyInstance(get(), nullptr);
    _setNull();
}