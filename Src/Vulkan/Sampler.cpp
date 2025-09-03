#include "PchVulkan.h"
#include "Sampler.h"
#include "Vulkan.h"

using namespace vk;

void CSampler::create(cptr<CDevice> vDevice, const VkSamplerCreateInfo& vInfo)
{
    destroy();

    m_pDevice = vDevice;
    vk::checkError(vkCreateSampler(*m_pDevice, &vInfo, nullptr, _getPtr()));
}

void CSampler::destroy()
{
    if (!isValid()) return;
    if (get()) vkDestroySampler(*m_pDevice, get(), nullptr);
    _setNull();
    m_pDevice = nullptr;
}