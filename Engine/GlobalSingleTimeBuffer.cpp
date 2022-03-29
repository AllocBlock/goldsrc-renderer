#include "GlobalSingleTimeBuffer.h"
#include "Vulkan.h"
#include "Command.h"

CCommand gCommand;

void registerGlobalCommandBuffer(VkDevice vDevice, uint32_t vQueueIndex)
{
    gCommand.createPool(vDevice, ECommandType::RESETTABLE, vQueueIndex);

    Vulkan::beginSingleTimeBufferFunc_t BeginFunc = []() -> VkCommandBuffer
    {
        return gCommand.beginSingleTimeBuffer();
    };
    Vulkan::endSingleTimeBufferFunc_t EndFunc = [](VkCommandBuffer vCommandBuffer)
    {
        gCommand.endSingleTimeBuffer(vCommandBuffer);
    };
    Vulkan::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void unregisterGlobalCommandBuffer()
{
    gCommand.clear();
    Vulkan::removeSingleTimeBufferFunc();
}
