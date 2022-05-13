#include "GlobalSingleTimeBuffer.h"
#include "Vulkan.h"
#include "Command.h"

using namespace vk;

CCommand gCommand;

void setupGlobalCommandBuffer(CDevice::CPtr vDevice, uint32_t vQueueIndex)
{
    gCommand.createPool(vDevice, ECommandType::RESETTABLE, vQueueIndex);

    vk::beginSingleTimeBufferFunc_t BeginFunc = []() -> VkCommandBuffer
    {
        return gCommand.beginSingleTimeBuffer();
    };
    vk::endSingleTimeBufferFunc_t EndFunc = [](VkCommandBuffer vCommandBuffer)
    {
        gCommand.endSingleTimeBuffer(vCommandBuffer);
    };
    vk::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void cleanGlobalCommandBuffer()
{
    gCommand.clear();
    vk::removeSingleTimeBufferFunc();
}
