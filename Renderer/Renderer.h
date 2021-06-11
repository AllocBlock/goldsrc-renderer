#pragma once
#include "Common.h"
#include "Command.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

enum ERendererPos
{
    BEGIN = 0x01,
    END = 0x02
};

class CRenderer
{
public:
    CRenderer() = default;

    void init(GLFWwindow* vWindow, const Common::SVulkanAppInfo & vAppInfo, int vRenderPassPosBitField);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>&vImageViews);
    void update(uint32_t vImageIndex);
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void destroy();

protected:
    virtual void _initV() {}
    virtual void _recreateV() {}
    virtual void _updateV(uint32_t vImageIndex) = 0;
    virtual VkCommandBuffer _requestCommandBufferV(uint32_t vImageIndex) = 0;

    GLFWwindow* m_pWindow = nullptr;
    Common::SVulkanAppInfo m_AppInfo;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    int m_RenderPassPosBitField = 0;

private:
    void __createRenderPass();
    void __destroyRenderPass();
};

