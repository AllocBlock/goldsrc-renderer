#pragma once
#include "vulkan/vulkan.h"
#include "Device.h"

void setupGlobalCommandBuffer(vk::CDevice::CPtr vDevice, uint32_t vQueueIndex);
void cleanGlobalCommandBuffer();