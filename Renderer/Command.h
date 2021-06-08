#pragma once
#include "Common.h"

#include <filesystem>
#include <optional>
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
    ~CCommand() { clear(); }

    void createPool(VkDevice vDevice, ECommandType vType, uint32_t vQueueIndex)
    {
        _ASSERTE(vDevice != VK_NULL_HANDLE);
        if (m_CommandPool != VK_NULL_HANDLE)
            clear();

        m_Device = vDevice;
        m_QueueIndex = vQueueIndex;
        vkGetDeviceQueue(m_Device, m_QueueIndex, 0, &m_Queue);

        VkCommandPoolCreateInfo PoolInfo = {};
        PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        PoolInfo.flags = static_cast<VkCommandPoolCreateFlagBits>(vType);
        PoolInfo.queueFamilyIndex = vQueueIndex;

        ck(vkCreateCommandPool(m_Device, &PoolInfo, nullptr, &m_CommandPool));
    }

    void createBuffers(std::string vName, uint32_t vNum, ECommandBufferLevel vLevel)
    {
        if (m_NameToBufferSetMap.find(vName) != m_NameToBufferSetMap.end())
        {
            __freeBufferSet(m_NameToBufferSetMap.at(vName));
        }
        else
        {
            m_NameToBufferSetMap[vName] = {};
        }
        auto& BuffsetSet = m_NameToBufferSetMap.at(vName);
        BuffsetSet.resize(vNum);
        __allocBuffer(vNum, vLevel, BuffsetSet.data());
    }

    VkCommandBuffer getCommandBuffer(std::string vName, uint32_t vIndex) const
    {
        _ASSERTE(m_NameToBufferSetMap.find(vName) != m_NameToBufferSetMap.end());
        const auto& BufferSet = m_NameToBufferSetMap.at(vName);
        _ASSERTE(vIndex < BufferSet.size());
        return BufferSet[vIndex];
    }

    size_t getCommandBufferSize(std::string vName) const
    {
        _ASSERTE(m_NameToBufferSetMap.find(vName) != m_NameToBufferSetMap.end());
        const auto& BufferSet = m_NameToBufferSetMap.at(vName);
        return BufferSet.size();
    }

    void clear()
    {
        if (m_InUseSingleTimeNum != 0) throw "there still is in use single time command buffer";
        __destoryPool();
        m_Device = VK_NULL_HANDLE;
        m_Queue = VK_NULL_HANDLE;
        m_QueueIndex = std::numeric_limits<uint32_t>::max();
    }

    VkCommandBuffer beginSingleTimeBuffer()
    {
        if (m_CommandPool == VK_NULL_HANDLE) throw "create command pool first";

        VkCommandBuffer CommandBuffer;
        __allocBuffer(1, ECommandBufferLevel::PRIMARY, &CommandBuffer);

        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        m_InUseSingleTimeNum++;

        return CommandBuffer;
    }

    void endSingleTimeBuffer(VkCommandBuffer vCommandBuffer)
    {
        if (m_CommandPool == VK_NULL_HANDLE) throw "create command pool first";

        ck(vkEndCommandBuffer(vCommandBuffer));

        VkSubmitInfo SubmitInfo = {};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &vCommandBuffer;

        ck(vkQueueSubmit(m_Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
        ck(vkQueueWaitIdle(m_Queue));

        vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &vCommandBuffer);
        m_InUseSingleTimeNum--;
    }

private:
    void __allocBuffer(uint32_t vNum, ECommandBufferLevel vLevel, VkCommandBuffer* voData)
    {
        VkCommandBufferAllocateInfo BufferAllocInfo = {};
        BufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        BufferAllocInfo.commandPool = m_CommandPool;
        BufferAllocInfo.level = static_cast<VkCommandBufferLevel>(vLevel);
        BufferAllocInfo.commandBufferCount = vNum;

        ck(vkAllocateCommandBuffers(m_Device, &BufferAllocInfo, voData));
    }

    void __destoryPool()
    {
        __freeAllBufferSet();
        if (m_CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }
    }

    void __freeAllBufferSet()
    {
        for (auto& Pair : m_NameToBufferSetMap)
            __freeBufferSet(Pair.second);
        m_NameToBufferSetMap.clear();
    }

    void __freeBufferSet(std::vector<VkCommandBuffer>& voBufferSet)
    {
        vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(voBufferSet.size()), voBufferSet.data());
        voBufferSet.clear();
    }

    VkDevice m_Device = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    uint32_t m_QueueIndex = std::numeric_limits<uint32_t>::max();
    VkQueue m_Queue = VK_NULL_HANDLE;
    std::map<std::string, std::vector<VkCommandBuffer>> m_NameToBufferSetMap;
    size_t m_InUseSingleTimeNum = 0;
};

