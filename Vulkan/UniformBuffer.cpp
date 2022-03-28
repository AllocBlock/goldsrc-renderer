#include "UniformBuffer.h"
#include "Vulkan.h"

using namespace vk;

void CUniformBuffer::create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkDeviceSize vSize)
{
    destroy();
    m_pBuffer = make<CBuffer>();
    m_pBuffer->create(vPhysicalDevice, vDevice, vSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    m_Handle = m_pBuffer->get();
}

void CUniformBuffer::update(const void* vData)
{
    if (!m_pBuffer || !m_pBuffer->isValid()) throw "Cant fill in NULL handle buffer";
    m_pBuffer->fill(vData, m_pBuffer->getSize());
}

void CUniformBuffer::destroy()
{
    if (m_pBuffer) m_pBuffer->destroy();
    m_Handle = VK_NULL_HANDLE;
}