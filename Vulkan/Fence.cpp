#include "Fence.h"
#include "Vulkan.h"

using namespace vk;

void CFence::create(VkDevice vDevice, bool vSigned)
{
    destroy();
    m_Device = vDevice;

    VkFenceCreateInfo FenceInfo = {};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vSigned)
        FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    Vulkan::checkError(vkCreateFence(vDevice, &FenceInfo, nullptr, &m_Handle));
}

void CFence::destroy()
{
    if (m_Handle) vkDestroyFence(m_Device, m_Handle, nullptr);
    m_Handle = VK_NULL_HANDLE;
}

void CFence::wait(uint64_t vTimeout)
{
    Vulkan::checkError(vkWaitForFences(m_Device, 1, &m_Handle, VK_TRUE, vTimeout));
}

void CFence::reset()
{
    Vulkan::checkError(vkResetFences(m_Device, 1, &m_Handle));
}
