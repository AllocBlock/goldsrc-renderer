#pragma once
#include "vulkan/vulkan.h"

void registerGlobalCommandBuffer(VkDevice vDevice, uint32_t vQueueIndex);
void unregisterGlobalCommandBuffer();