#pragma once
#include "PchVulkan.h"
#include "Device.h"
#include "CommandBuffer.h"

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

        void create(CDevice::CPtr vDevice, const VkImageCreateInfo& vImageInfo, VkMemoryPropertyFlags vProperties, const SImageViewInfo& vViewInfo);
        void createFromImage(CDevice::CPtr vDevice, VkImage vImage, VkFormat vFormat, uint32_t vLayerCount, const SImageViewInfo& vViewInfo);
        void destroy();
        bool isValid() const override;
        void copyFromBuffer(CCommandBuffer::Ptr vCommandBuffer, VkBuffer vBuffer, size_t vWidth, size_t vHeight);
        void stageFill(const void* vData, VkDeviceSize vSize, bool vToShaderLayout = true);
        void transitionLayout(CCommandBuffer::Ptr vCommandBuffer, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vStartMipLevel = 0u, uint32_t vMipLevelCount = std::numeric_limits<uint32_t>::max());
        void copyToBuffer(CCommandBuffer::Ptr vCommandBuffer, const VkBufferImageCopy& vCopyRegion, VkBuffer vTargetBuffer);
        void generateMipmaps(CCommandBuffer::Ptr vCommandBuffer);
        uint32_t getMipmapLevelNum() const { return m_MipmapLevelNum; }

        VkImage getImage() const { return m_Image; }
        VkImageLayout getLayout() const { return m_Layout; }
        uint32_t getWidth() const { return m_Width; }
        uint32_t getHeight() const { return m_Height; }
        VkExtent2D getExtent() const { return { m_Width, m_Height }; }
        uint32_t getLayerCount() const { return m_LayerCount; }
        VkFormat getFormat() const { return m_Format; }

    private:
        void __createImageView(CDevice::CPtr vDevice, const SImageViewInfo& vViewInfo);

        bool m_IsOuterImage = false;
        CDevice::CPtr m_pDevice = nullptr;
        VkImage m_Image = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        uint32_t m_Width = 0, m_Height = 0, m_LayerCount = 0;
        VkFormat m_Format = VkFormat::VK_FORMAT_UNDEFINED;
        VkImageLayout m_Layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED; // TODO: mip can has different layout at different level, so only one layout info can not handle this
        uint32_t m_MipmapLevelNum = 1;
    };

}
