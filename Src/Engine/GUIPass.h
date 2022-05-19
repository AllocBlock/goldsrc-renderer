#pragma once
#include "Command.h"
#include "IRenderPass.h"
#include "FrameBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class CGUIRenderPass : public vk::IRenderPass
{
public:
    CGUIRenderPass() = default;

    GLFWwindow* getWindow() { return m_pWindow; }
    void setWindow(GLFWwindow* vWindow) { m_pWindow = vWindow; }

    void beginFrame(std::string vTitle = u8"´°¿Ú");
    void endFrame();

protected:
    virtual void _initV() override;
    virtual CRenderPassPort _getPortV() override;
    virtual void _recreateV() override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    GLFWwindow* m_pWindow = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Main";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;

    bool m_Begined = false;

private:
    void __createRenderPass();
    void __createDescriptorPool();
    void __destroyDescriptorPool();
    void __createFramebuffer();
    void __createRecreateSources();
    void __destroyRecreateSources();
};