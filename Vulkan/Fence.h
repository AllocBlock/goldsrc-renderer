#pragma once
#include "VulkanHandle.h"
#include "Device.h"

namespace vk
{
    class CFence : public IVulkanHandle<VkFence>
    {
    public:
        _DEFINE_PTR(CFence);

        void create(CDevice::CPtr vDevice, bool vSigned);
        void destroy();
        void wait(uint64_t vTimeout = std::numeric_limits<uint64_t>::max());
        void reset();
    private:
        CDevice::CPtr m_pDevice = nullptr;
    };
}
