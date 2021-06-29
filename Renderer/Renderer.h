#pragma once
#include "Common.h"
#include "Command.h"

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

class CRenderer
{
public:
    CRenderer() = default;

    void init(const Common::SVulkanAppInfo& vAppInfo, int vRenderPassPosBitField);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vTargetImageViews);
    void update(uint32_t vImageIndex);
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void destroy();

    static VkAttachmentDescription createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, EImageType vType);

protected:
    virtual void _initV() {}
    virtual void _recreateV() {}
    virtual void _updateV(uint32_t vImageIndex) = 0;
    virtual VkCommandBuffer _requestCommandBufferV(uint32_t vImageIndex) = 0;
    virtual void _destroyV() {};

    Common::SVulkanAppInfo m_AppInfo;
    int m_RenderPassPosBitField = 0;
};

