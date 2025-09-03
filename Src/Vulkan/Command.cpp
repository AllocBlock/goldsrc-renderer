#include "PchVulkan.h"
#include "Command.h"
#include "Log.h"

using namespace vk;

CCommand::~CCommand()
{
    clear();
}

void CCommand::createPool(cptr<CDevice> vDevice, ECommandType vType, uint32_t vQueueIndex)
{
    _ASSERTE(vDevice != VK_NULL_HANDLE);
    if (vQueueIndex == std::numeric_limits<uint32_t>::max()) // use graphics queue index in device
        vQueueIndex = vDevice->getGraphicsQueueIndex();

    if (m_CommandPool != VK_NULL_HANDLE)
        clear();

    m_pDevice = vDevice;
    m_QueueIndex = vQueueIndex;
    m_Queue = m_pDevice->getQueue(vQueueIndex);

    VkCommandPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.flags = static_cast<VkCommandPoolCreateFlagBits>(vType);
    PoolInfo.queueFamilyIndex = vQueueIndex;

    vk::checkError(vkCreateCommandPool(*m_pDevice, &PoolInfo, nullptr, &m_CommandPool));

    Log::logCreation("command pool", uint64_t(m_CommandPool));
}

void CCommand::createBuffers(std::string vName, ECommandBufferLevel vLevel)
{
    if (m_NameToBufferMap.find(vName) != m_NameToBufferMap.end())
    {
        __freeBuffer(m_NameToBufferMap.at(vName));
        m_NameToBufferMap.erase(vName);
    }

    m_NameToBufferMap[vName] = __allocBuffer(vLevel, false);

#ifdef _DEBUG
    Log::logCreation("command buffer", uint64_t(m_NameToBufferMap[vName]->get()));
#endif
}

sptr<CCommandBuffer> CCommand::getCommandBuffer(std::string vName) const
{
    _ASSERTE(m_NameToBufferMap.find(vName) != m_NameToBufferMap.end());
    return m_NameToBufferMap.at(vName);
}

sptr<CCommandBuffer> CCommand::beginSingleTimeBuffer()
{
    if (m_CommandPool == VK_NULL_HANDLE) throw "create command pool first";

    auto pCommandBuffer = __allocBuffer(ECommandBufferLevel::PRIMARY, true);
    pCommandBuffer->begin();

    m_InUseSingleTimeNum++;
    return pCommandBuffer;
}

void CCommand::endSingleTimeBuffer(sptr<CCommandBuffer>& vioCommandBuffer)
{
    if (m_CommandPool == VK_NULL_HANDLE) throw "create command pool first";

    VkCommandBuffer Buffer = vioCommandBuffer->get();

    vk::checkError(vkEndCommandBuffer(Buffer));

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &Buffer;

    vk::checkError(vkQueueSubmit(m_Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
    vk::checkError(vkQueueWaitIdle(m_Queue));

    vkFreeCommandBuffers(*m_pDevice, m_CommandPool, 1, &Buffer);
    m_InUseSingleTimeNum--;

    vioCommandBuffer->markAsOutdate();
    vioCommandBuffer = nullptr;
}

void CCommand::clear()
{
    if (m_InUseSingleTimeNum != 0) throw "there still is in use single time command buffer";
    __destoryPool();
    m_Queue = VK_NULL_HANDLE;
    m_QueueIndex = std::numeric_limits<uint32_t>::max();
    m_pDevice = nullptr;
}

void CCommand::setDebugName(const std::string& vName) const
{
    for (const auto& pair : m_NameToBufferMap)
    {
        m_pDevice->setObjectDebugName(VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)(pair.second->get()), vName + "-" + pair.first);
    }
}

sptr<CCommandBuffer> CCommand::__allocBuffer(ECommandBufferLevel vLevel, bool vIsSingleTime)
{
    VkCommandBufferAllocateInfo BufferAllocInfo = {};
    BufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    BufferAllocInfo.commandPool = m_CommandPool;
    BufferAllocInfo.level = static_cast<VkCommandBufferLevel>(vLevel);
    BufferAllocInfo.commandBufferCount = 1;

    VkCommandBuffer buffer;
    vk::checkError(vkAllocateCommandBuffers(*m_pDevice, &BufferAllocInfo, &buffer));
    return make<CCommandBuffer>(buffer, vIsSingleTime, vLevel);
}

void CCommand::__destoryPool()
{
    __freeAllBuffer();
    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(*m_pDevice, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }
}

void CCommand::__freeAllBuffer()
{
    for (auto& Pair : m_NameToBufferMap)
    {
        __freeBuffer(Pair.second);
    }
    m_NameToBufferMap.clear();
}

void CCommand::__freeBuffer(sptr<CCommandBuffer> vpBufferSet)
{
    auto buffer = vpBufferSet->get();
    vkFreeCommandBuffers(*m_pDevice, m_CommandPool, 1, &buffer);
}