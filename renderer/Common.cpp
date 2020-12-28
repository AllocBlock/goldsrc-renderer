#include "Common.h"

VkCommandBuffer Common::beginSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool)
{
    VkCommandBufferAllocateInfo CommandBufferAllocInfo = {};
    CommandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocInfo.commandPool = vCommandPool;
    CommandBufferAllocInfo.commandBufferCount = 1;

    VkCommandBuffer CommandBuffer;
    ck(vkAllocateCommandBuffers(vDevice, &CommandBufferAllocInfo, &CommandBuffer));

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    return CommandBuffer;
}

void Common::endSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool, VkQueue vGraphicQueue, VkCommandBuffer vCommandBuffer)
{
    ck(vkEndCommandBuffer(vCommandBuffer));

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &vCommandBuffer;

    ck(vkQueueSubmit(vGraphicQueue, 1, &SubmitInfo, VK_NULL_HANDLE));
    ck(vkQueueWaitIdle(vGraphicQueue));

    vkFreeCommandBuffers(vDevice, vCommandPool, 1, &vCommandBuffer);
}
