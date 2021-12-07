#include "Buffer.h"
#include "Vulkan.h"

using namespace vk;

void CBuffer::create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties)
{
    destroy();

    if (vSize == 0) throw "Size == 0";
    m_PhysicalDevice = vPhysicalDevice;
    m_Device = vDevice;
    m_Size = vSize;
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = vSize;
    BufferInfo.usage = vUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Vulkan::checkError(vkCreateBuffer(vDevice, &BufferInfo, nullptr, &m_Handle));

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements(vDevice, m_Handle, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = Vulkan::findMemoryType(vPhysicalDevice, MemRequirements.memoryTypeBits, vProperties);

    Vulkan::checkError(vkAllocateMemory(vDevice, &AllocInfo, nullptr, &m_Memory));
    Vulkan::checkError(vkBindBufferMemory(vDevice, m_Handle, m_Memory, 0));
}

void CBuffer::destroy()
{
    if (!isValid()) return;

    vkDestroyBuffer(m_Device, m_Handle, nullptr);
    vkFreeMemory(m_Device, m_Memory, nullptr);
    m_Handle = VK_NULL_HANDLE;
    m_Memory = VK_NULL_HANDLE;
    m_Size = 0;

    m_Device = VK_NULL_HANDLE;
    m_PhysicalDevice = VK_NULL_HANDLE;
}

bool CBuffer::isValid()
{
    return m_Handle != VK_NULL_HANDLE && m_Memory != VK_NULL_HANDLE;
}

void CBuffer::copyFrom(VkCommandBuffer vCommandBuffer, VkBuffer vSrcBuffer, VkDeviceSize vSize)
{
    _ASSERTE(isValid());
    if (vSize > m_Size) throw "Size overflowed";
    VkBufferCopy CopyRegion = {};
    CopyRegion.size = vSize;
    vkCmdCopyBuffer(vCommandBuffer, vSrcBuffer, m_Handle, 1, &CopyRegion);
}

void CBuffer::copyToHost(VkDeviceSize vSize, void* vPtr)
{
    void* pDevData;
    Vulkan::checkError(vkMapMemory(m_Device, m_Memory, 0, vSize, 0, &pDevData));
    memcpy(vPtr, reinterpret_cast<char*>(pDevData), vSize);
    vkUnmapMemory(m_Device, m_Memory);
}

void CBuffer::fill(const void* vData, VkDeviceSize vSize)
{
    if (!isValid()) throw "Cant fill in NULL handle buffer";
    else if (m_Size < vSize) throw "Cant fill in smaller buffer";

    void* pDevData;
    Vulkan::checkError(vkMapMemory(m_Device, m_Memory, 0, vSize, 0, &pDevData));
    memcpy(reinterpret_cast<char*>(pDevData), vData, vSize);
    vkUnmapMemory(m_Device, m_Memory);
}

void CBuffer::stageFill(const void* vData, VkDeviceSize vSize)
{
    if (!isValid()) throw "Cant fill in NULL handle buffer";
    else if (m_Size < vSize) throw "Cant fill in smaller buffer";
    CBuffer StageBuffer;
    StageBuffer.create(m_PhysicalDevice, m_Device, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    StageBuffer.fill(vData, vSize);

    VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
    copyFrom(CommandBuffer, StageBuffer.get(), vSize);
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    StageBuffer.destroy();
}
