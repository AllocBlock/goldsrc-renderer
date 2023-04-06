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
    _ASSERTE(vNum > 0);
    if (m_NameToBufferSetMap.find(vName) != m_NameToBufferSetMap.end())
    {
        std::vector<VkCommandBuffer> RawBufferSet;
        for (const auto& pBuffer : m_NameToBufferSetMap.at(vName))
            RawBufferSet.emplace_back(pBuffer->get());
        __freeBufferSet(RawBufferSet);
    }
    else
    {
        m_NameToBufferSetMap[vName] = {};
    }

    std::vector<VkCommandBuffer> RawBufferSet(vNum);
    __allocBuffer(vNum, vLevel, RawBufferSet.data());

    bool IsSecondary = (vLevel == ECommandBufferLevel::SECONDARY);

    m_NameToBufferSetMap[vName].resize(vNum, nullptr);
    for (size_t i = 0; i < vNum; ++i)
    {
        m_NameToBufferSetMap[vName][i] = make<CCommandBuffer>(RawBufferSet[i], false, IsSecondary);
    }
}

CCommandBuffer::Ptr CCommand::getCommandBuffer(std::string vName, uint32_t vIndex) const
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

CCommandBuffer::Ptr CCommand::beginSingleTimeBuffer()
{
    if (m_CommandPool == VK_NULL_HANDLE) throw "create command pool first";

    VkCommandBuffer CommandBuffer;
    __allocBuffer(1, ECommandBufferLevel::PRIMARY, &CommandBuffer);

    CCommandBuffer::Ptr pCommandBuffer = make<CCommandBuffer>(CommandBuffer, true, false);
    pCommandBuffer->begin();

    m_InUseSingleTimeNum++;
    return pCommandBuffer;
}

void CCommand::endSingleTimeBuffer(CCommandBuffer::Ptr& vioCommandBuffer)
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
    {
        std::vector<VkCommandBuffer> RawBufferSet;
        for (const auto& pBuffer : Pair.second)
            RawBufferSet.emplace_back(pBuffer->get());
        __freeBufferSet(RawBufferSet);
    }
    m_NameToBufferSetMap.clear();
}

void CCommand::__freeBufferSet(std::vector<VkCommandBuffer>& voBufferSet)
{
    vkFreeCommandBuffers(*m_pDevice, m_CommandPool, static_cast<uint32_t>(voBufferSet.size()), voBufferSet.data());
    voBufferSet.clear();
}