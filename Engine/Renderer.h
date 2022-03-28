#pragma once
#include "Vulkan.h"
#include "Command.h"
#include "GUI.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

enum ERendererPos
{
    BEGIN = 0x01,
    END = 0x02
};

enum class EImageType
{
    COLOR,
    DEPTH
};

class IRenderer : public IGUI
{
public:
    IRenderer() = default;

    void init(const Vulkan::SVulkanAppInfo& vAppInfo, int vRenderPassPosBitField);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vTargetImageViews);
    void update(uint32_t vImageIndex);
    std::vector<VkCommandBuffer> requestCommandBuffers(uint32_t vImageIndex);
    void destroy();

    static VkAttachmentDescription createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, EImageType vType);

protected:
    virtual void _initV() {}
    virtual void _recreateV() {}
    virtual void _updateV(uint32_t vImageIndex) {}
    virtual void _renderUIV() override {}
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) = 0;
    virtual void _destroyV() {};

    Vulkan::SVulkanAppInfo m_AppInfo;
    int m_RenderPassPosBitField = 0;
};

