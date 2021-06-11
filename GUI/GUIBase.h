#pragma once
#include "Command.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class CGUIBase
{
public:
    CGUIBase() = default;

    void init(GLFWwindow* vWindow, const Common::SVulkanAppInfo& vAppInfo, bool vHasRenderer);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void update(uint32_t vImageIndex);
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void destroy();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);

protected:
    virtual void _initV();
    virtual void _updateV(uint32_t vImageIndex);

    GLFWwindow* m_pWindow = nullptr;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Main";
    VkFormat m_ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
    std::vector<VkImageView> m_TargetImageViewSet;
    VkExtent2D m_Extent = { 0, 0 };

    uint32_t m_GraphicsQueueIndex = 0;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> m_FrameBufferSet;

private:
    void __createDescriptorPool();
    void __createRecreateSources();
    void __destroyRecreateSources();
};