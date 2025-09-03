#pragma once
#include "Device.h"
#include "CommandBuffer.h"

namespace SingleTimeCommandBuffer
{
    void setup(cptr<vk::CDevice> vDevice, uint32_t vQueueIndex);
    void clean();
    sptr<CCommandBuffer> begin();
    void end(sptr<CCommandBuffer>& vioCommandBuffer);
}