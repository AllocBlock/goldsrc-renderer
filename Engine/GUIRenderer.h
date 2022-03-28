#pragma once
#include "Command.h"
#include "Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class CGUIRenderer : public IRenderer
{
public:
    CGUIRenderer() = default;

    GLFWwindow* getWindow() { return m_pWindow; }
    void setWindow(GLFWwindow* vWindow) { m_pWindow = vWindow; }

    void beginFrame(std::string vTitle = u8"´°¿Ú");
    void endFrame();

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    GLFWwindow* m_pWindow = nullptr;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Main";
    std::vector<VkFramebuffer> m_FrameBufferSet;

    bool m_Begined = false;

private:
    void __createRenderPass();
    void __destroyRenderPass();
    void __createDescriptorPool();
    void __destroyDescriptorPool();
    void __createRecreateSources();
    void __destroyRecreateSources();
};