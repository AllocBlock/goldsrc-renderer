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

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SGoldSrcPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SGoldSrcPointData, Pos));
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SGoldSrcPointData, Color));
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SGoldSrcPointData, Normal));
        Descriptor.add(VK_FORMAT_R32G32_SFLOAT, offsetof(SGoldSrcPointData, TexCoord));
        Descriptor.add(VK_FORMAT_R32G32_SFLOAT, offsetof(SGoldSrcPointData, LightmapCoord));
        Descriptor.add(VK_FORMAT_R32_UINT, offsetof(SGoldSrcPointData, TexIndex));
        return Descriptor.generate();
    }
};

struct SSimplePointData
{
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    uint32_t TexIndex;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SSimplePointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SSimplePointData, Pos));
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SSimplePointData, Normal));
        Descriptor.add(VK_FORMAT_R32G32_SFLOAT, offsetof(SSimplePointData, TexCoord));
        Descriptor.add(VK_FORMAT_R32_UINT, offsetof(SSimplePointData, TexIndex));
        return Descriptor.generate();
    }
};
