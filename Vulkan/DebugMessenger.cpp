#include "DebugMessenger.h"
#include "Vulkan.h"
#include <iostream>

using namespace vk;

void CDebugMessenger::create(VkInstance vInstance)
{
    m_Instance = vInstance;

    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerInfo = {};
    DebugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    DebugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    DebugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    DebugMessengerInfo.pfnUserCallback = CDebugMessenger::__handleMessage;
    DebugMessengerInfo.pUserData = this;

    auto pCreateDebugFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vInstance, "vkCreateDebugUtilsMessengerEXT"));
    if (pCreateDebugFunc == nullptr)
        throw std::runtime_error(u8"不支持调试函数");
    else
        Vulkan::checkError(pCreateDebugFunc(vInstance, &DebugMessengerInfo, nullptr, &m_Handle));
}

void CDebugMessenger::destroy()
{
    auto pDestroyDebugFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (pDestroyDebugFunc == nullptr)
        throw std::runtime_error(u8"不支持调试函数");
    else
        pDestroyDebugFunc(m_Instance, m_Handle, nullptr);
    m_Handle = VK_NULL_HANDLE;
}

void CDebugMessenger::setCustomCallback(DebugMessageCallbackFunc_t vCallback)
{
    m_pCustomCallback = vCallback;
}

EDebugMessageServerity convertVulkanSeverityToCustomEnum(VkDebugUtilsMessageSeverityFlagBitsEXT vSeverity)
{
    switch (vSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return EDebugMessageServerity::VERBOSE;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return EDebugMessageServerity::INFO;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return EDebugMessageServerity::WARNING;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return EDebugMessageServerity::ERROR;
    default: throw std::runtime_error("不支持的枚举");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL CDebugMessenger::__handleMessage(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData)
{
    const CDebugMessenger* pDebugger = reinterpret_cast<CDebugMessenger*>(vpUserData);

    static std::string LastMessage = "";
    if (LastMessage != vpCallbackData->pMessage)
    {
        LastMessage = vpCallbackData->pMessage;
        std::cerr << "[验证层] " << LastMessage << std::endl;
        if (pDebugger->m_pCustomCallback)
            pDebugger->m_pCustomCallback(convertVulkanSeverityToCustomEnum(vMessageSeverity), vpCallbackData->pMessage);
    }

    return VK_FALSE;
}