#pragma once
#include "PchVulkan.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "Surface.h"
#include "Image.h"
#include <vector>
#include <GLFW/glfw3.h>

namespace vk
{
    class CSwapchain : public IVulkanHandle<VkSwapchainKHR>
    {
    public:
        _DEFINE_PTR(CSwapchain);

        void create(CDevice::Ptr vDevice, CPhysicalDevice::Ptr vPhysicalDevice, CSurface::Ptr vSurface, GLFWwindow* vWindow);
        void destroy();
        VkExtent2D getExtent();
        VkFormat getImageFormat();
        const std::vector<VkImageView>& getImageViews();
        size_t getImageNum() { return m_ImageViewSet.size(); }

    private:
        VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats);
        VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes);
        VkExtent2D __chooseSwapExtent(GLFWwindow* vWindow, const VkSurfaceCapabilitiesKHR& vCapabilities);

        CDevice::Ptr m_pDevice = nullptr;
        std::vector<vk::CImage::Ptr> m_ImageSet;
        std::vector<VkImageView> m_ImageViewSet;
        VkFormat m_ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent = { 0, 0 };
    };
}

