#pragma once
#include "VertexAttributeDescriptor.h"

#include <glm/glm.hpp>

struct SGoldSrcPointData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec2 LightmapCoord;
    uint32_t TexIndex;

    using PointData_t = SGoldSrcPointData;
    _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
        Descriptor.add(_GET_ATTRIBUTE_INFO(Color));
        Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
        Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
        Descriptor.add(_GET_ATTRIBUTE_INFO(LightmapCoord));
        Descriptor.add(_GET_ATTRIBUTE_INFO(TexIndex));
        return Descriptor.generate();
    }
};

struct SSimplePointData
{
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    uint32_t TexIndex;

    using PointData_t = SSimplePointData;
    _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
        Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
        Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
        Descriptor.add(_GET_ATTRIBUTE_INFO(TexIndex));
        return Descriptor.generate();
    }
};
