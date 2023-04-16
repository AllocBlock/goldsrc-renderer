#include "PchVulkan.h"
#include "SingleTimeCommandBuffer.h"
#include "Command.h"

using namespace vk;

bool gIsSetup = false;
CCommand gCommand;

void SingleTimeCommandBuffer::setup(CDevice::CPtr vDevice, uint32_t vQueueIndex)
{
    _ASSERTE(!gIsSetup);
    gCommand.createPool(vDevice, ECommandType::RESETTABLE, vQueueIndex);
    gIsSetup = true;
}

void SingleTimeCommandBuffer::clean()
{
    _ASSERTE(gIsSetup);
    gCommand.clear();
    gIsSetup = false;
}

CCommandBuffer::Ptr SingleTimeCommandBuffer::beginSingleTimeBuffer()
{
    _ASSERTE(gIsSetup);
    return gCommand.beginSingleTimeBuffer();
}

void SingleTimeCommandBuffer::endSingleTimeBuffer(CCommandBuffer::Ptr& vioCommandBuffer)
{
    _ASSERTE(gIsSetup);
    gCommand.endSingleTimeBuffer(vioCommandBuffer);
}
