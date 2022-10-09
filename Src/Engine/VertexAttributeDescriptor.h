#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

#define _DEFINE_GET_FORMAT_FUNC(type, format) static VkFormat getAttributeFormat(type* vAttribute) {return format;}

class CVertexAttributeDescriptor
{
public:
    std::vector<VkVertexInputAttributeDescription> generate();
    void add(VkFormat vFormat, uint32_t vOffset);
    void clear();
    bool isEmpty() { return m_FormatSet.empty(); }

    _DEFINE_GET_FORMAT_FUNC(float, VK_FORMAT_R32_SFLOAT);
    _DEFINE_GET_FORMAT_FUNC(glm::vec2, VK_FORMAT_R32G32_SFLOAT);
    _DEFINE_GET_FORMAT_FUNC(glm::vec3, VK_FORMAT_R32G32B32_SFLOAT);
    _DEFINE_GET_FORMAT_FUNC(uint32_t, VK_FORMAT_R32_UINT);
    _DEFINE_GET_FORMAT_FUNC(int, VK_FORMAT_R32_SINT);
private:
    std::vector<VkFormat> m_FormatSet;
    std::vector<uint32_t> m_OffsetSet;
};

#undef _DEFINE_GET_FORMAT_FUNC

VkVertexInputBindingDescription createBindingDescription(size_t vSize);

// using PointData_t = custom struct type before using these macros
#define _GET_ATTRIBUTE_FORMAT(varName) CVertexAttributeDescriptor::getAttributeFormat(&((PointData_t*)0)->varName)
#define _GET_ATTRIBUTE_INFO(varName) _GET_ATTRIBUTE_FORMAT(varName), offsetof(PointData_t, varName)
#define _DEFINE_GET_BINDING_DESCRIPTION_FUNC static VkVertexInputBindingDescription getBindingDescription() { return createBindingDescription(sizeof(PointData_t)); }