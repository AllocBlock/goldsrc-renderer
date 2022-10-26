#pragma once
#include "Command.h"
#include "RenderPass.h"
#include "FrameBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class CRenderPassGUI final : public vk::IRenderPass
{
public:
    CRenderPassGUI() = default;

    GLFWwindow* getWindow() { return m_pWindow; }
    void setWindow(GLFWwindow* vWindow) { m_pWindow = vWindow; }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createDescriptorPool();
    void __destroyDescriptorPool();
    void __createFramebuffer(VkExtent2D vExtent);

    GLFWwindow* m_pWindow = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
};