#pragma once
#include "VertexAttributeDescriptor.h"

struct SFullScreenPointData
{
    glm::vec2 Pos;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SFullScreenPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(VK_FORMAT_R32G32_SFLOAT, offsetof(SFullScreenPointData, Pos));
        return Descriptor.generate();
    }
};