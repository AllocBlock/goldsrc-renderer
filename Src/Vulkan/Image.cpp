#include "PchVulkan.h"
#include "Image.h"
#include "Vulkan.h"
#include "Buffer.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "Log.h"
#include "SingleTimeCommandBuffer.h"

using namespace vk;

void CImage::create(cptr<CDevice> vDevice, const VkImageCreateInfo& vImageInfo, VkMemoryPropertyFlags vProperties, const SImageViewInfo& vViewInfo)
{
    destroy();

    m_pDevice = vDevice;
    m_Width = vImageInfo.extent.width;
    m_Height = vImageInfo.extent.height;
    m_LayerCount = vImageInfo.arrayLayers;
    m_Format = vImageInfo.format;
    m_Layout = vImageInfo.initialLayout;
    m_MipmapLevelNum = vImageInfo.mipLevels;

    if (m_MipmapLevelNum > 1 && !(vImageInfo.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
    {
        throw std::runtime_error("mipmap texture must has VK_IMAGE_USAGE_TRANSFER_SRC_BIT to generate mip levels");
    }

    vk::checkError(vkCreateImage(*vDevice, &vImageInfo, nullptr, &m_Image));

    VkMemoryRequirements MemRequirements;
    vkGetImageMemoryRequirements(*vDevice, m_Image, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = vDevice->getPhysicalDevice()->findMemoryTypeIndex(MemRequirements.memoryTypeBits, vProperties);

    vk::checkError(vkAllocateMemory(*vDevice, &AllocInfo, nullptr, &m_Memory));

    vk::checkError(vkBindImageMemory(*vDevice, m_Image, m_Memory, 0));

    m_IsOuterImage = false;
    __createImageView(vDevice, vViewInfo);

    Log::logCreation("image", uint64_t(m_Image));
    Log::logCreation("memory", uint64_t(m_Memory));
}

void CImage::createFromImage(cptr<CDevice> vDevice, VkImage vImage, VkFormat vFormat, uint32_t vLayerCount, const SImageViewInfo& vViewInfo)
{
    destroy();

    m_pDevice = vDevice;
    m_Format = vFormat;
    m_LayerCount = vLayerCount;

    m_IsOuterImage = true;
    m_Image = vImage;
    __createImageView(vDevice, vViewInfo);
}

void CImage::destroy()
{
    if (!isValid()) return;
    if (!m_IsOuterImage)
    {
        vkDestroyImage(*m_pDevice, m_Image, nullptr);
        vkFreeMemory(*m_pDevice, m_Memory, nullptr);
    }
    vkDestroyImageView(*m_pDevice, get(), nullptr);
    m_Image = VK_NULL_HANDLE;
    m_Memory = VK_NULL_HANDLE;
    _setNull();
    m_Width = m_Height = m_LayerCount = 0;
    m_Format = VkFormat::VK_FORMAT_UNDEFINED;
    m_Layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

    m_pDevice = nullptr;

    m_IsOuterImage = false;
}

bool CImage::isValid() const
{
    if (m_IsOuterImage) return m_Image != VK_NULL_HANDLE && get() != VK_NULL_HANDLE;
    else return m_Image != VK_NULL_HANDLE && m_Memory != VK_NULL_HANDLE && get() != VK_NULL_HANDLE;
}

void CImage::copyFromBuffer(sptr<CCommandBuffer> vCommandBuffer, VkBuffer vBuffer, size_t vWidth, size_t vHeight)
{
    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0;
    Region.bufferImageHeight = 0;

    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = m_LayerCount;

    Region.imageOffset = { 0, 0, 0 };
    Region.imageExtent = { static_cast<uint32_t>(vWidth), static_cast<uint32_t>(vHeight), 1 };

    vCommandBuffer->copyBufferToImage(vBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Region);
    m_Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

void CImage::stageFill(const void* vData, VkDeviceSize vSize, bool vToShaderLayout)
{
    if (!isValid()) throw "Cant fill in NULL handle image";

    CBuffer StageBuffer;
    StageBuffer.create(m_pDevice, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    StageBuffer.fill(vData, vSize);

    sptr<CCommandBuffer> pCommandBuffer = SingleTimeCommandBuffer::begin();
    transitionLayout(pCommandBuffer, m_Layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyFromBuffer(pCommandBuffer, StageBuffer.get(), m_Width, m_Height);
    if (vToShaderLayout)
        transitionLayout(pCommandBuffer, m_Layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    SingleTimeCommandBuffer::end(pCommandBuffer);

    StageBuffer.destroy();
}

void CImage::copyToBuffer(sptr<CCommandBuffer> vCommandBuffer, const VkBufferImageCopy& vCopyRegion, VkBuffer vTargetBuffer)
{
    VkImageLayout OriginalLayout = m_Layout;
    transitionLayout(vCommandBuffer, m_Layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vCommandBuffer->copyImageToBuffer(m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vTargetBuffer, vCopyRegion);
    transitionLayout(vCommandBuffer, m_Layout, OriginalLayout);
}

void vk::CImage::setDebugName(const std::string& vName) const
{
    m_pDevice->setObjectDebugName(VkObjectType::VK_OBJECT_TYPE_IMAGE, (uint64_t)(getImage()), vName);
    m_pDevice->setObjectDebugName(VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)(get()), vName);
}

VkAccessFlags __toAccessFlags(VkImageLayout vLayout)
{
    switch (vLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return 0;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_ACCESS_SHADER_READ_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    default:
        throw std::runtime_error("Unsupported layout");
    }
}

// TODO: to limited, especially for shader stage...better design?
VkPipelineStageFlags __tPipelineStageFlags(VkImageLayout vLayout)
{
    switch (vLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    default:
        throw std::runtime_error("Unsupported layout");
    }
}

void CImage::transitionLayout(sptr<CCommandBuffer> vCommandBuffer, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vStartMipLevel, uint32_t vMipLevelCount)
{
    if (!isValid()) throw "NULL image handle";

    _ASSERTE(vOldLayout != vNewLayout); // should not be the same

    vMipLevelCount = std::min<uint32_t>(vMipLevelCount, m_MipmapLevelNum - vStartMipLevel);

    VkImageMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = vOldLayout;
    Barrier.newLayout = vNewLayout;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = m_Image;

    if (vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || vOldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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

    Barrier.subresourceRange.baseMipLevel = vStartMipLevel;
    Barrier.subresourceRange.levelCount = vMipLevelCount;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = m_LayerCount;

    Barrier.srcAccessMask = __toAccessFlags(vOldLayout);
    Barrier.dstAccessMask = __toAccessFlags(vNewLayout);

    VkPipelineStageFlags SrcStage = __tPipelineStageFlags(vOldLayout);
    VkPipelineStageFlags DestStage = __tPipelineStageFlags(vNewLayout);
    vCommandBuffer->addImageMemoryBarrier(SrcStage, DestStage, Barrier);
    m_Layout = vNewLayout;
}

void CImage::__createImageView(cptr<CDevice> vDevice, const SImageViewInfo& vViewInfo)
{
    VkImageViewCreateInfo ImageViewInfo = {};
    ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewInfo.image = m_Image;
    ImageViewInfo.viewType = vViewInfo.ViewType;
    ImageViewInfo.format = m_Format;
    ImageViewInfo.subresourceRange.aspectMask = vViewInfo.AspectFlags;
    ImageViewInfo.subresourceRange.baseMipLevel = 0;
    ImageViewInfo.subresourceRange.levelCount = m_MipmapLevelNum;
    ImageViewInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewInfo.subresourceRange.layerCount = m_LayerCount;

    vk::checkError(vkCreateImageView(*vDevice, &ImageViewInfo, nullptr, _getPtr()));

    Log::logCreation("image view", uint64_t(get()));
}

void CImage::generateMipmaps(sptr<CCommandBuffer> vCommandBuffer)
{
    _ASSERTE(m_MipmapLevelNum > 1);
    _ASSERTE(m_Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Check if image format supports linear blitting
    VkFormatProperties FormatProperties = m_pDevice->getPhysicalDevice()->getFormatProperty(m_Format);

    if (!(FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkImageMemoryBarrier Barrier{};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.image = m_Image;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = 1;
    Barrier.subresourceRange.levelCount = 1;

    int32_t MipWidth = m_Width;
    int32_t MipHeight = m_Height;

    for (uint32_t i = 1; i < m_MipmapLevelNum; i++) {
        // set blit src to src layout
        Barrier.subresourceRange.baseMipLevel = i - 1;
        Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vCommandBuffer->addImageMemoryBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, Barrier);

        VkImageBlit Blit = {};
        Blit.srcOffsets[0] = { 0, 0, 0 };
        Blit.srcOffsets[1] = { MipWidth, MipHeight, 1 };
        Blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Blit.srcSubresource.mipLevel = i - 1;
        Blit.srcSubresource.baseArrayLayer = 0;
        Blit.srcSubresource.layerCount = 1;
        Blit.dstOffsets[0] = { 0, 0, 0 };
        Blit.dstOffsets[1] = { MipWidth > 1 ? MipWidth / 2 : 1, MipHeight > 1 ? MipHeight / 2 : 1, 1 };
        Blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Blit.dstSubresource.mipLevel = i;
        Blit.dstSubresource.baseArrayLayer = 0;
        Blit.dstSubresource.layerCount = 1;

        vCommandBuffer->blitImage(
            m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            Blit, VK_FILTER_LINEAR
        );

        Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vCommandBuffer->addImageMemoryBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, Barrier);

        if (MipWidth > 1) MipWidth /= 2;
        if (MipHeight > 1) MipHeight /= 2;
    }

    Barrier.subresourceRange.baseMipLevel = m_MipmapLevelNum - 1;
    Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vCommandBuffer->addImageMemoryBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, Barrier);

    m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}