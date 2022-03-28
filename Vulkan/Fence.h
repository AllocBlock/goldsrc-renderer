#pragma once
#include "VulkanHandle.h"

namespace vk
{
    class CFence : public IVulkanHandle<VkFence>
    {
    public:
        _DEFINE_PTR(CFence);

        void create(VkDevice vDevice, bool vSigned);
        void destroy();
        void wait(uint64_t vTimeout = std::numeric_limits<uint64_t>::max());
        void reset();
    private:
        VkDevice m_Device = VK_NULL_HANDLE;
    };
}
