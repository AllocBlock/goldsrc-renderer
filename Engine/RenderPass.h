#pragma once
#include "Vulkan.h"
#include "Command.h"
#include "GUI.h"
#include "RenderPassPort.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

enum ERendererPos
{
    MIDDLE = 0x00,
    BEGIN = 0x01,
    END = 0x02
};

enum class EImageType
{
    COLOR,
    DEPTH
};

class IRenderPass : public IGUI
{
public:
    IRenderPass() = default;

    void init(const Vulkan::SVulkanAppInfo& vAppInfo, int vRenderPassPosBitField);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vTargetImageViews);
    void update(uint32_t vImageIndex);
    std::vector<VkCommandBuffer> requestCommandBuffers(uint32_t vImageIndex);
    void destroy();

    static VkAttachmentDescription createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, EImageType vType);

protected:
    virtual void _initV() {}
    virtual CRenderPassPort _getPortV() { return CRenderPassPort(); }
    virtual void _recreateV() {}
    virtual void _updateV(uint32_t vImageIndex) {}
    virtual void _renderUIV() override {}
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) = 0;
    virtual void _destroyV() {};

    Vulkan::SVulkanAppInfo m_AppInfo;
    ptr<CRenderPassLink> m_pLink;
    int m_RenderPassPosBitField = 0;
};

