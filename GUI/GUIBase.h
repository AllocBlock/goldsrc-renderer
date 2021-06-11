#pragma once
#include "Command.h"
#include "Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class CGUIBase : public CRenderer
{
public:
    CGUIBase() = default;

    void destroy();

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual VkCommandBuffer _requestCommandBufferV(uint32_t vImageIndex) override;

    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Main";
    std::vector<VkFramebuffer> m_FrameBufferSet;

private:
    void __createDescriptorPool();
    void __createRecreateSources();
    void __destroyRecreateSources();
};