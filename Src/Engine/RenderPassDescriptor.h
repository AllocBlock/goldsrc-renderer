#pragma once
#include "RenderPass.h"
#include "vulkan/vulkan.h"
#include <vector>

// TIPS: before create with generated info, you should not release this descriptor as it contains data pointed in renderpass create info
class CRenderPassDescriptor
{
public:
    void addColorAttachment(int vRendererPosBitField, VkFormat vImageFormat);
    void setDepthAttachment(int vRendererPosBitField, VkFormat vImageFormat);
    void setSubpassNum(uint32_t vNum);
    void clear();
    VkRenderPassCreateInfo generateInfo();

    // FIXME: cant create many pass info then use them all, as prev data will be erased
    static VkRenderPassCreateInfo generateSingleSubpassInfo(int vRendererPosBitField, VkFormat vColorImageFormat, VkFormat vDepthImageFormat = VK_FORMAT_UNDEFINED);
private:
    static CRenderPassDescriptor gGlobalDesc;

    VkAttachmentDescription __createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, bool vIsDepth);
    void __generateDependency(); // TODO: this just generates a sequence dependency
    void __generateDescription(); // TODO: this just generates a full reference dependency

    uint32_t m_ColorAttachmentNum = 0;
    bool m_HasDepthAttachment = false;
    std::vector<VkAttachmentDescription> m_AttachmentDescSet;
    uint32_t m_SubPassNum = 1u;

    std::vector<VkAttachmentReference> m_StageColorRefSet; // avoid local point problem
    VkAttachmentReference m_StageDepthRef;
    std::vector<VkSubpassDescription> m_StageDescSet;
    std::vector<VkSubpassDependency> m_StageDepedencySet;
};