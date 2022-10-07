#include "RenderPassPort.h"

const SPortFormat& SPortFormat::AnyFormat = SPortFormat::createAnyOfUsage(EUsage::DONT_CARE);

VkImageLayout toLayout(EUsage vUsage, bool isDepth)
{
    switch (vUsage)
    {
    case EUsage::DONT_CARE: throw std::runtime_error("Error: can not convert DONT_CARE usage to layout");
    case EUsage::UNDEFINED: return VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    case EUsage::READ: return VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case EUsage::WRITE: return isDepth ? VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case EUsage::PRESENTATION: return VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default: throw std::runtime_error("Error: unknown usage");
    }
}