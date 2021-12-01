#pragma once
#include "VulkanHandle.h"

namespace vk
{
    struct SImageViewInfo
    {
        SImageViewInfo() = default;

        VkImageAspectFlags AspectFlags = 0;
        VkImageViewType ViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        uint32_t LayerCount = 1;
    };

    class CImage : public IVulkanHandle<VkImageView>
    {
    public:
        void create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, const VkImageCreateInfo& vImageInfo, VkMemoryPropertyFlags vProperties, const SImageViewInfo& vViewInfo);
        void setImage(VkDevice vDevice, VkImage vImage, VkFormat vFormat, const SImageViewInfo& vViewInfo);
        void destroy();
        bool isValid();
        void copyFromBuffer(VkCommandBuffer vCommandBuffer, VkBuffer vBuffer, size_t vWidth, size_t vHeight, uint32_t vLayerCount);
        void stageFill(VkDevice vDevice, const void* vData, VkDeviceSize vSize);
        void transitionLayout(VkCommandBuffer vCommandBuffer, VkImageLayout vNewLayout);

    private:
        void __createImageView(VkDevice vDevice, const SImageViewInfo& vViewInfo, VkFormat vFormat);

        bool m_IsSet = false;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkImage m_Image = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        uint32_t m_Width = 0, m_Height = 0, m_LayerCount = 0;
        VkFormat m_Format = VkFormat::VK_FORMAT_UNDEFINED;
        VkImageLayout m_Layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    };

}
