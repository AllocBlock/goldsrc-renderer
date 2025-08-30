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

class CCommand
{
public:
    CCommand() = default;
    ~CCommand();

    void createPool(vk::CDevice::CPtr vDevice, ECommandType vType, uint32_t vQueueIndex = std::numeric_limits<uint32_t>::max());
    void createBuffers(std::string vName, ECommandBufferLevel vLevel);
    CCommandBuffer::Ptr getCommandBuffer(std::string vName) const;
    CCommandBuffer::Ptr beginSingleTimeBuffer();
    void endSingleTimeBuffer(CCommandBuffer::Ptr& vioCommandBuffer);
    void clear();

private:
    CCommandBuffer::Ptr __allocBuffer(ECommandBufferLevel vLevel, bool vIsSingleTime);
    void __destoryPool();
    void __freeAllBuffer();
    void __freeBuffer(CCommandBuffer::Ptr vpBufferSet);

    vk::CDevice::CPtr m_pDevice = nullptr;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    uint32_t m_QueueIndex = std::numeric_limits<uint32_t>::max();
    VkQueue m_Queue = VK_NULL_HANDLE;
    std::map<std::string, CCommandBuffer::Ptr> m_NameToBufferMap;
    size_t m_InUseSingleTimeNum = 0;
};