#include "PchVulkan.h"
#include "SingleTimeCommandBuffer.h"
#include "Command.h"

using namespace vk;

bool gIsSetup = false;
CCommand gCommand;

void SingleTimeCommandBuffer::setup(cptr<CDevice> vDevice, uint32_t vQueueIndex)
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

sptr<CCommandBuffer> SingleTimeCommandBuffer::begin()
{
    _ASSERTE(gIsSetup);
    return gCommand.beginSingleTimeBuffer();
}

void SingleTimeCommandBuffer::end(sptr<CCommandBuffer>& vioCommandBuffer)
{
    _ASSERTE(gIsSetup);
    gCommand.endSingleTimeBuffer(vioCommandBuffer);
}
