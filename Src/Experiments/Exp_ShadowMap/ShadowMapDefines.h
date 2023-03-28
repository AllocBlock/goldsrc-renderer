#pragma once
#ifndef SHADOW_MAP_DEFINES_H
#define SHADOW_MAP_DEFINES_H
#include <vulkan/vulkan.h>

const VkFormat gShadowMapImageFormat = VkFormat::VK_FORMAT_R32_SFLOAT;
const uint32_t gShadowMapSize = 2048u;

#endif
