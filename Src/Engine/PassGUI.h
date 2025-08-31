#pragma once
#include "Command.h"
#include "RenderPass.h"
#include "RenderInfoDescriptor.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class CRenderPassGUI final : public engine::IRenderPass
{
public:
    inline static const std::string Name = "Gui";

    CRenderPassGUI(GLFWwindow* vWindow);
    GLFWwindow* getWindow() { return m_pWindow; }

protected:
    virtual void _initV() override;
    virtual CPortSet::Ptr _createPortSetV() override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;

private:
    void __createDescriptorPool();
    void __destroyDescriptorPool();

    GLFWwindow* m_pWindow = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    CRenderInfoDescriptor m_RenderInfoDescriptor;
};