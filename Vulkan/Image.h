#pragma once
#include "VulkanHandle.h"
#include "PhysicalDevice.h"
#include "Device.h"

namespace vk
{
    struct SImageViewInfo
    {
        SImageViewInfo() = default;

        VkImageAspectFlags AspectFlags = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageViewType ViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    };

    class CImage : public IVulkanHandle<VkImageView>
    {
    public:
        _DEFINE_PTR(CImage);

        void create(CPhysicalDevice::CPtr vPhysicalDevice, CDevice::CPtr vDevice, const VkImageCreateInfo& vImageInfo, VkMemoryPropertyFlags vProperties, const SImageViewInfo& vViewInfo);
        void setImage(CDevice::CPtr vDevice, VkImage vImage, VkFormat vFormat, uint32_t vLayerCount, const SImageViewInfo& vViewInfo);
        void destroy();
        bool isValid();
        void copyFromBuffer(VkCommandBuffer vCommandBuffer, VkBuffer vBuffer, size_t vWidth, size_t vHeight);
        void stageFill(const void* vData, VkDeviceSize vSize, bool vToShaderLayout = true);
        void transitionLayout(VkCommandBuffer vCommandBuffer, VkImageLayout vNewLayout, uint32_t vStartMipLevel = 0u, uint32_t vMipLevelCount = std::numeric_limits<uint32_t>::max());
        void copyToBuffer(VkCommandBuffer vCommandBuffer, const VkBufferImageCopy& vCopyRegion, VkBuffer vTargetBuffer);
        void generateMipmaps(VkCommandBuffer vCommandBuffer);
        uint32_t getMipmapLevelNum() { return m_MipmapLevelNum; }

        VkImage getImage() { return m_Image; }

    private:
        void __createImageView(CDevice::CPtr vDevice, const SImageViewInfo& vViewInfo);

        bool m_IsSet = false;
        CPhysicalDevice::CPtr m_pPhysicalDevice = nullptr;
        CDevice::CPtr m_pDevice = nullptr;
        VkImage m_Image = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        uint32_t m_Width = 0, m_Height = 0, m_LayerCount = 0;
        VkFormat m_Format = VkFormat::VK_FORMAT_UNDEFINED;
        VkImageLayout m_Layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED; // TODO: mip can has different layout at different level
        uint32_t m_MipmapLevelNum = 1;
    };

}
