#include "PchVulkan.h"
#include "UniformBuffer.h"
#include "Vulkan.h"

using namespace vk;

void CUniformBuffer::create(CDevice::CPtr vDevice, VkDeviceSize vSize)
{
    destroy();
    CBuffer::create(vDevice, vSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void CUniformBuffer::update(const void* vData)
{
    if (!isValid()) throw "Cant fill in NULL handle buffer";
    fill(vData, getSize());
}