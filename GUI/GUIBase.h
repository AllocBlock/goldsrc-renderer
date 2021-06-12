#pragma once
#include "Command.h"
#include "Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class CGUIBase : public CRenderer
{
public:
    CGUIBase() = default;

    GLFWwindow* getWindow() { return m_pWindow; }
    void setWindow(GLFWwindow* vWindow) { m_pWindow = vWindow; }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual VkCommandBuffer _requestCommandBufferV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    GLFWwindow* m_pWindow = nullptr;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Main";
    std::vector<VkFramebuffer> m_FrameBufferSet;

private:
    void __createRenderPass();
    void __destroyRenderPass();
    void __createDescriptorPool();
    void __destroyDescriptorPool();
    void __createRecreateSources();
    void __destroyRecreateSources();
};