#include "Image.h"
#include "Vulkan.h"
#include "Buffer.h"

using namespace vk;

void CImage::create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, const VkImageCreateInfo& vImageInfo, VkMemoryPropertyFlags vProperties, const SImageViewInfo& vViewInfo)
{
    destroy();

    m_PhysicalDevice = vPhysicalDevice;
    m_Device = vDevice;
    m_Width = vImageInfo.extent.width;
    m_Height = vImageInfo.extent.height;
    m_LayerCount = vViewInfo.LayerCount;
    m_Format = vImageInfo.format;
    m_Layout = vImageInfo.initialLayout;

    Vulkan::checkError(vkCreateImage(vDevice, &vImageInfo, nullptr, &m_Image));

    VkMemoryRequirements MemRequirements;
    vkGetImageMemoryRequirements(vDevice, m_Image, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = Vulkan::findMemoryType(vPhysicalDevice, MemRequirements.memoryTypeBits, vProperties);

    Vulkan::checkError(vkAllocateMemory(vDevice, &AllocInfo, nullptr, &m_Memory));

    Vulkan::checkError(vkBindImageMemory(vDevice, m_Image, m_Memory, 0));

#ifdef _DEBUG
    std::cout << "create image 0x" << std::setbase(16) << (uint64_t)(m_Image) << std::setbase(10) << std::endl;
    std::cout << "create memory 0x" << std::setbase(16) << (uint64_t)(m_Memory) << std::setbase(10) << std::endl;
#endif

    __createImageView(vDevice, vViewInfo, vImageInfo.format);
}

void CImage::setImage(VkDevice vDevice, VkImage vImage, VkFormat vFormat, const SImageViewInfo& vViewInfo)
{
    destroy();

    m_Device = vDevice;
    m_Format = vFormat;

    m_IsSet = true;
    m_Image = vImage;
    __createImageView(vDevice, vViewInfo, vFormat);
}

void CImage::destroy()
{
    if (!isValid()) return;
    if (!m_IsSet)
    {
        vkDestroyImage(m_Device, m_Image, nullptr);
        vkFreeMemory(m_Device, m_Memory, nullptr);
    }
    vkDestroyImageView(m_Device, m_Handle, nullptr);
    m_Image = VK_NULL_HANDLE;
    m_Memory = VK_NULL_HANDLE;
    m_Handle = VK_NULL_HANDLE;
    m_Width = m_Height = m_LayerCount = 0;
    m_Format = VkFormat::VK_FORMAT_UNDEFINED;
    m_Layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

    m_Device = VK_NULL_HANDLE;
    m_PhysicalDevice = VK_NULL_HANDLE;

    m_IsSet = false;
}

bool CImage::isValid()
{
    if (m_IsSet) return m_Image != VK_NULL_HANDLE && m_Handle != VK_NULL_HANDLE;
    else return m_Image != VK_NULL_HANDLE && m_Memory != VK_NULL_HANDLE && m_Handle != VK_NULL_HANDLE;
}

void CImage::copyFromBuffer(VkCommandBuffer vCommandBuffer, VkBuffer vBuffer, size_t vWidth, size_t vHeight, uint32_t vLayerCount)
{
    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0;
    Region.bufferImageHeight = 0;

    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = vLayerCount;

    Region.imageOffset = { 0, 0, 0 };
    Region.imageExtent = { static_cast<uint32_t>(vWidth), static_cast<uint32_t>(vHeight), 1 };

    vkCmdCopyBufferToImage(vCommandBuffer, vBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
}

void CImage::stageFill(VkDevice vDevice, const void* vData, VkDeviceSize vSize)
{
    if (!isValid()) throw "Cant fill in NULL handle image";

    CBuffer StageBuffer;
    StageBuffer.create(m_PhysicalDevice, vDevice, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    StageBuffer.fill(vDevice, vData, vSize);

    VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
    transitionLayout(CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyFromBuffer(CommandBuffer, StageBuffer.get(), m_Width, m_Height, m_LayerCount);
    transitionLayout(CommandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    StageBuffer.destroy();
}

void CImage::transitionLayout(VkCommandBuffer vCommandBuffer, VkImageLayout vNewLayout)
{
    if (!isValid()) throw "NULL image handle";

    VkImageMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = m_Layout;
    Barrier.newLayout = vNewLayout;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = m_Image;

    if (vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        bool hasStencilComponent = (m_Format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_Format == VK_FORMAT_D24_UNORM_S8_UINT);
        if (hasStencilComponent)
        {
            Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = m_LayerCount;

    VkPipelineStageFlags SrcStage;
    VkPipelineStageFlags DestStage;

    if (m_Layout == VK_IMAGE_LAYOUT_UNDEFINED
        && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (m_Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (m_Layout == VK_IMAGE_LAYOUT_UNDEFINED
        && vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (m_Layout == VK_IMAGE_LAYOUT_UNDEFINED
        && vNewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (m_Layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (m_Layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else
    {
        throw std::runtime_error(u8"不支持该布局转换");
    }

    vkCmdPipelineBarrier(
        vCommandBuffer,
        SrcStage, DestStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &Barrier
    );

    m_Layout = vNewLayout;
}

void CImage::__createImageView(VkDevice vDevice, const SImageViewInfo& vViewInfo, VkFormat vFormat)
{
    VkImageViewCreateInfo ImageViewInfo = {};
    ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewInfo.image = m_Image;
    ImageViewInfo.viewType = vViewInfo.ViewType;
    ImageViewInfo.format = vFormat;
    ImageViewInfo.subresourceRange.aspectMask = vViewInfo.AspectFlags;
    ImageViewInfo.subresourceRange.baseMipLevel = 0;
    ImageViewInfo.subresourceRange.levelCount = 1;
    ImageViewInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewInfo.subresourceRange.layerCount = vViewInfo.LayerCount;

    Vulkan::checkError(vkCreateImageView(vDevice, &ImageViewInfo, nullptr, &m_Handle));

#ifdef _DEBUG
    std::cout << "create image view 0x" << std::setbase(16) << (uint64_t)(m_Handle) << std::setbase(10) << std::endl;
#endif
}