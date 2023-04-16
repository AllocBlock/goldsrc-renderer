#pragma once
#include "Device.h"
#include "CommandBuffer.h"

namespace SingleTimeCommandBuffer
{
    void setup(vk::CDevice::CPtr vDevice, uint32_t vQueueIndex);
    void clean();
    CCommandBuffer::Ptr beginSingleTimeBuffer();
    void endSingleTimeBuffer(CCommandBuffer::Ptr& vioCommandBuffer);
}