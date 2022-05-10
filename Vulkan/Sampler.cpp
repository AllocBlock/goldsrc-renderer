#include "Sampler.h"
#include "Vulkan.h"

using namespace vk;

void CSampler::create(VkDevice vDevice, const VkSamplerCreateInfo& vInfo)
{
    destroy();

    m_Device = vDevice;
    Vulkan::checkError(vkCreateSampler(m_Device, &vInfo, nullptr, &m_Handle));
}

void CSampler::destroy()
{
    if (m_Handle) vkDestroySampler(m_Device, m_Handle, nullptr);
    m_Handle = VK_NULL_HANDLE;
    m_Device = VK_NULL_HANDLE;
}