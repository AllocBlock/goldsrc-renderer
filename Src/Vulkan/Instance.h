#pragma once
#include "PchVulkan.h"
#include <string>
#include <vector>

namespace vk
{
    class CInstance : public IVulkanHandle<VkInstance>
    {
    public:
        
        void create(std::string vName, const std::vector<const char*>& vValidationLayers = {}, const std::vector<const char*>& vExtensions = {});
        void destroy();
    };
}


