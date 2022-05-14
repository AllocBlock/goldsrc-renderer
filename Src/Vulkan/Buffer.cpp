#include "PchVulkan.h"
#include "Buffer.h"
#include "Vulkan.h"

using namespace vk;

void CBuffer::create(CPhysicalDevice::CPtr vPhysicalDevice, CDevice::CPtr vDevice, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties)
{
    destroy();

    if (vSize == 0) throw "Size == 0";
    m_pPhysicalDevice = vPhysicalDevice;
    m_pDevice = vDevice;
    m_Size = vSize;
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = vSize;
    BufferInfo.usage = vUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::checkError(vkCreateBuffer(*m_pDevice, &BufferInfo, nullptr, _getPtr()));

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements(*m_pDevice, get(), &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = vPhysicalDevice->findMemoryTypeIndex(MemRequirements.memoryTypeBits, vProperties);

    vk::checkError(vkAllocateMemory(*m_pDevice, &AllocInfo, nullptr, &m_Memory));
    vk::checkError(vkBindBufferMemory(*m_pDevice, get(), m_Memory, 0));
}

void CBuffer::destroy()
{
    if (!isValid()) return;

    vkDestroyBuffer(*m_pDevice, get(), nullptr);
    vkFreeMemory(*m_pDevice, m_Memory, nullptr);
    _setNull();
    m_Memory = VK_NULL_HANDLE;
    m_Size = 0;

    m_pDevice = nullptr;
    m_pPhysicalDevice = nullptr;
}

bool CBuffer::isValid()
{
    return IVulkanHandle::isValid() && m_Memory != VK_NULL_HANDLE;
}

void CBuffer::copyFrom(VkCommandBuffer vCommandBuffer, VkBuffer vSrcBuffer, VkDeviceSize vSize)
{
    _ASSERTE(isValid());
    if (vSize > m_Size) throw "Size overflowed";
    VkBufferCopy CopyRegion = {};
    CopyRegion.size = vSize;
    vkCmdCopyBuffer(vCommandBuffer, vSrcBuffer, get(), 1, &CopyRegion);
}

void CBuffer::copyToHost(VkDeviceSize vSize, void* vPtr)
{
    void* pDevData;
    vk::checkError(vkMapMemory(*m_pDevice, m_Memory, 0, vSize, 0, &pDevData));
    memcpy(vPtr, reinterpret_cast<char*>(pDevData), vSize);
    vkUnmapMemory(*m_pDevice, m_Memory);
}

void CBuffer::fill(const void* vData, VkDeviceSize vSize)
{
    if (!isValid()) throw "Cant fill in NULL handle buffer";
    else if (m_Size < vSize) throw "Cant fill in smaller buffer";

    void* pDevData;
    vk::checkError(vkMapMemory(*m_pDevice, m_Memory, 0, vSize, 0, &pDevData));
    memcpy(reinterpret_cast<char*>(pDevData), vData, vSize);
    vkUnmapMemory(*m_pDevice, m_Memory);
}

void CBuffer::stageFill(const void* vData, VkDeviceSize vSize)
{
    if (!isValid()) throw "Cant fill in NULL handle buffer";
    else if (m_Size < vSize) throw "Cant fill in smaller buffer";
    CBuffer StageBuffer;
    StageBuffer.create(m_pPhysicalDevice, m_pDevice, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    StageBuffer.fill(vData, vSize);

    VkCommandBuffer CommandBuffer = vk::beginSingleTimeBuffer();
    copyFrom(CommandBuffer, StageBuffer.get(), vSize);
    vk::endSingleTimeBuffer(CommandBuffer);

    StageBuffer.destroy();
}

VkDeviceSize CBuffer::getSize()
{
    return m_Size;
}