#include "Command.h"
#include "Vulkan.h"

using namespace vk;

CCommand::~CCommand()
{
    clear();
}

void CCommand::createPool(CDevice::CPtr vDevice, ECommandType vType, uint32_t vQueueIndex)
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

//#ifdef _DEBUG
//    static int Count = 0;
//    std::cout << "create command pool [" << Count << "] = 0x" << std::setbase(16) << (uint64_t)(m_CommandPool) << std::setbase(10) << std::endl;
//    Count++;
//#endif
}

void CCommand::createBuffers(std::string vName, uint32_t vNum, ECommandBufferLevel vLevel)
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

VkCommandBuffer CCommand::getCommandBuffer(std::string vName, uint32_t vIndex) const
{
    _ASSERTE(m_NameToBufferSetMap.find(vName) != m_NameToBufferSetMap.end());
    const auto& BufferSet = m_NameToBufferSetMap.at(vName);
    _ASSERTE(vIndex < BufferSet.size());
    return BufferSet[vIndex];
}

size_t CCommand::getCommandBufferSize(std::string vName) const
{
    _ASSERTE(m_NameToBufferSetMap.find(vName) != m_NameToBufferSetMap.end());
    const auto& BufferSet = m_NameToBufferSetMap.at(vName);
    return BufferSet.size();
}

VkCommandBuffer CCommand::beginSingleTimeBuffer()
{
    if (m_CommandPool == VK_NULL_HANDLE) throw "create command pool first";

    VkCommandBuffer CommandBuffer;
    __allocBuffer(1, ECommandBufferLevel::PRIMARY, &CommandBuffer);

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk::checkError(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    m_InUseSingleTimeNum++;

    return CommandBuffer;
}

void CCommand::endSingleTimeBuffer(VkCommandBuffer vCommandBuffer)
{
    if (m_CommandPool == VK_NULL_HANDLE) throw "create command pool first";

    vk::checkError(vkEndCommandBuffer(vCommandBuffer));

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &vCommandBuffer;

    vk::checkError(vkQueueSubmit(m_Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
    vk::checkError(vkQueueWaitIdle(m_Queue));

    vkFreeCommandBuffers(*m_pDevice, m_CommandPool, 1, &vCommandBuffer);
    m_InUseSingleTimeNum--;
}

void CCommand::clear()
{
    if (m_InUseSingleTimeNum != 0) throw "there still is in use single time command buffer";
    __destoryPool();
    m_pDevice = VK_NULL_HANDLE;
    m_Queue = VK_NULL_HANDLE;
    m_QueueIndex = std::numeric_limits<uint32_t>::max();
}

void CCommand::__allocBuffer(uint32_t vNum, ECommandBufferLevel vLevel, VkCommandBuffer* voData)
{
    VkCommandBufferAllocateInfo BufferAllocInfo = {};
    BufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    BufferAllocInfo.commandPool = m_CommandPool;
    BufferAllocInfo.level = static_cast<VkCommandBufferLevel>(vLevel);
    BufferAllocInfo.commandBufferCount = vNum;

    vk::checkError(vkAllocateCommandBuffers(*m_pDevice, &BufferAllocInfo, voData));
}

void CCommand::__destoryPool()
{
    __freeAllBufferSet();
    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(*m_pDevice, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }
}

void CCommand::__freeAllBufferSet()
{
    for (auto& Pair : m_NameToBufferSetMap)
        __freeBufferSet(Pair.second);
    m_NameToBufferSetMap.clear();
}

void CCommand::__freeBufferSet(std::vector<VkCommandBuffer>& voBufferSet)
{
    vkFreeCommandBuffers(*m_pDevice, m_CommandPool, static_cast<uint32_t>(voBufferSet.size()), voBufferSet.data());
    voBufferSet.clear();
}