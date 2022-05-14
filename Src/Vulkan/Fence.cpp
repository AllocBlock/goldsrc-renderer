#include "PchVulkan.h"
#include "Fence.h"
#include "Vulkan.h"
#include "Device.h"

using namespace vk;

void CFence::create(CDevice::CPtr vDevice, bool vSigned)
{
    destroy();
    m_pDevice = vDevice;

    VkFenceCreateInfo FenceInfo = {};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vSigned)
        FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vk::checkError(vkCreateFence(*vDevice, &FenceInfo, nullptr, _getPtr()));
}

void CFence::destroy()
{
    if (get()) vkDestroyFence(*m_pDevice, get(), nullptr);
    _setNull();
}

void CFence::wait(uint64_t vTimeout)
{
    vk::checkError(vkWaitForFences(*m_pDevice, 1, getConstPtr(), VK_TRUE, vTimeout));
}

void CFence::reset()
{
    vk::checkError(vkResetFences(*m_pDevice, 1, getConstPtr()));
}
