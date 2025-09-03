#pragma once
#include "PchVulkan.h"
#include "Device.h"

namespace vk
{
    class CFence : public IVulkanHandle<VkFence>
    {
    public:
        
        void create(cptr<CDevice> vDevice, bool vSigned);
        void destroy();
        void wait(uint64_t vTimeout = std::numeric_limits<uint64_t>::max());
        void reset();
    private:
        cptr<CDevice> m_pDevice = nullptr;
    };
}
