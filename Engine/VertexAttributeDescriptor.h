#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class VertexAttributeDescriptor
{
public:
    std::vector<VkVertexInputAttributeDescription> generate();
    void add(VkFormat vFormat, uint32_t vOffset);
    void clear();

private:
    std::vector<VkFormat> m_FormatSet;
    std::vector<uint32_t> m_OffsetSet;
};

