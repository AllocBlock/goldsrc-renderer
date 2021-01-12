#pragma once

#include "Interactor.h"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <string>
#include <vector>
#include <memory>
#include <queue>

class CImguiVullkan
{
public:
    CImguiVullkan() = delete;
    CImguiVullkan(std::shared_ptr<CInteractor> vpInteractor) : m_pInteractor(vpInteractor) {}

    void init(VkInstance vInstance, VkPhysicalDevice vPhysicalDevice, VkDevice vDevice,uint32_t vGraphicQueueFamily, VkQueue vGraphicQueue, GLFWwindow* vpWindow, VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void render();
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void updateFramebuffers(VkExtent2D vExtent, const std::vector<VkImageView>&vImageViews);
    void destroy();
    void showAlert(std::string vText);

private:
    void __createDescriptorPool();

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamily = 0;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    GLFWwindow* m_pWindow = nullptr;
    VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = { 0, 0 };
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkFramebuffer> m_FrameBuffers;

    std::shared_ptr<CInteractor> m_pInteractor = nullptr;

    bool m_IgnoreAllAlert = false;
    std::queue<std::string> m_AlertTexts;
};