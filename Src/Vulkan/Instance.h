#pragma once
#include "VulkanHandle.h"
#include <string>
#include <vector>

namespace vk
{
    class CInstance : public IVulkanHandle<VkInstance>
    {
    public:
        _DEFINE_PTR(CInstance);

        void create(std::string vName, const std::vector<const char*>& vValidationLayers = {}, const std::vector<const char*>& vExtensions = {});
        void destroy();
    };
}


