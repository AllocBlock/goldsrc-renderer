#pragma once
#include "VertexAttributeDescriptor.h"
#include <glm/glm.hpp>

struct SFullScreenPointData
{
    glm::vec2 Pos;

    using PointData_t = SFullScreenPointData;
    _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        


        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
        return Descriptor.generate();
    }
};