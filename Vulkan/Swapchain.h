#pragma once
#include "VulkanHandle.h"
#include <vector>
#include <GLFW/glfw3.h>

namespace vk
{
    class CSwapchain : public IVulkanHandle<VkSwapchainKHR>
    {
    public:
        void create(VkDevice vDevice, VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface, GLFWwindow* vWindow);
        void destroy();
        VkExtent2D getExtent();
        VkFormat getImageFormat();
        const std::vector<VkImageView>& getImageViews();

    private:
        VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats);
        VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes);
        VkExtent2D __chooseSwapExtent(GLFWwindow* vWindow, const VkSurfaceCapabilitiesKHR& vCapabilities);
        void __createImageViews();
        void __destroyImageViews();

        VkDevice m_Device = VK_NULL_HANDLE;
        std::vector<VkImage> m_ImageSet;
        std::vector<VkImageView> m_ImageViewSet;
        VkFormat m_ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent = { 0, 0 };
    };
}

