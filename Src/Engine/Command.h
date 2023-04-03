#pragma once
#include "Device.h"
#include "CommandBuffer.h"

#include <filesystem>
#include <map>
#include <vulkan/vulkan.h> 

enum class ECommandType
{
    PROTECTED = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_PROTECTED_BIT,
    RESETTABLE = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
};

enum class ECommandBufferLevel
{
    PRIMARY = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    SECONDARY = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY
};

class CCommand
{
public:
    CCommand() = default;
    ~CCommand();

    void createPool(vk::CDevice::CPtr vDevice, ECommandType vType, uint32_t vQueueIndex = std::numeric_limits<uint32_t>::max());
    void createBuffers(std::string vName, uint32_t vNum, ECommandBufferLevel vLevel);
    CCommandBuffer::Ptr getCommandBuffer(std::string vName, uint32_t vIndex) const;
    size_t getCommandBufferSize(std::string vName) const;
    CCommandBuffer::Ptr beginSingleTimeBuffer();
    void endSingleTimeBuffer(CCommandBuffer::Ptr& vioCommandBuffer);
    void clear();

private:
    void __allocBuffer(uint32_t vNum, ECommandBufferLevel vLevel, VkCommandBuffer* voData);
    void __destoryPool();
    void __freeAllBufferSet();
    void __freeBufferSet(std::vector<VkCommandBuffer>& voBufferSet);

    vk::CDevice::CPtr m_pDevice = nullptr;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    uint32_t m_QueueIndex = std::numeric_limits<uint32_t>::max();
    VkQueue m_Queue = VK_NULL_HANDLE;
    std::map<std::string, std::vector<CCommandBuffer::Ptr>> m_NameToBufferSetMap;
    size_t m_InUseSingleTimeNum = 0;
};