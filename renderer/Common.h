#pragma once

#include <vulkan/vulkan.h>

#define ck(E) do{if (E) throw "vulkan error happened";} while(0)

namespace Common
{
    VkCommandBuffer beginSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool);
    void endSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool, VkQueue vGraphicQueue, VkCommandBuffer vCommandBuffer);
}
