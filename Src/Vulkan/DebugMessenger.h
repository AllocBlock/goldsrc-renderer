#pragma once
#include "VulkanHandle.h"
#include <functional>

namespace vk
{
    enum class EDebugMessageServerity
    {
        VERBOSE,
        INFO,
        WARNING,
        ERROR,
    };

    enum class EDebugMessageType
    {
        GENERAL,
        VALIDATION,
        PERFORMANCE
    };

    using DebugMessageCallbackFunc_t = std::function<void(EDebugMessageServerity, std::string)>;

    class CDebugMessenger : public IVulkanHandle<VkDebugUtilsMessengerEXT>
    {
    public:
        _DEFINE_PTR(CDebugMessenger);

        void create(VkInstance vInstance);
        void destroy();
        void setCustomCallback(DebugMessageCallbackFunc_t vCallback);

    private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL __handleMessage(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);
        VkInstance m_Instance = VK_NULL_HANDLE;
        DebugMessageCallbackFunc_t m_pCustomCallback = nullptr;
    };
}


