#pragma once

#include <glm/glm.hpp>

struct SPositionPointData
{
    glm::vec3 Pos;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SPositionPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptionSet(1);

        AttributeDescriptionSet[0].binding = 0;
        AttributeDescriptionSet[0].location = 0;
        AttributeDescriptionSet[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[0].offset = offsetof(SPositionPointData, Pos);

        return AttributeDescriptionSet;
    }
};

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
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptionSet(6);

        AttributeDescriptionSet[0].binding = 0;
        AttributeDescriptionSet[0].location = 0;
        AttributeDescriptionSet[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[0].offset = offsetof(SGoldSrcPointData, Pos);

        AttributeDescriptionSet[1].binding = 0;
        AttributeDescriptionSet[1].location = 1;
        AttributeDescriptionSet[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[1].offset = offsetof(SGoldSrcPointData, Color);

        AttributeDescriptionSet[2].binding = 0;
        AttributeDescriptionSet[2].location = 2;
        AttributeDescriptionSet[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[2].offset = offsetof(SGoldSrcPointData, Normal);

        AttributeDescriptionSet[3].binding = 0;
        AttributeDescriptionSet[3].location = 3;
        AttributeDescriptionSet[3].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptionSet[3].offset = offsetof(SGoldSrcPointData, TexCoord);

        AttributeDescriptionSet[4].binding = 0;
        AttributeDescriptionSet[4].location = 4;
        AttributeDescriptionSet[4].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptionSet[4].offset = offsetof(SGoldSrcPointData, LightmapCoord);

        AttributeDescriptionSet[5].binding = 0;
        AttributeDescriptionSet[5].location = 5;
        AttributeDescriptionSet[5].format = VK_FORMAT_R32_UINT;
        AttributeDescriptionSet[5].offset = offsetof(SGoldSrcPointData, TexIndex);

        return AttributeDescriptionSet;
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
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptionSet(4);
        uint32_t Index = 0;
        AttributeDescriptionSet[Index].binding = 0;
        AttributeDescriptionSet[Index].location = Index;
        AttributeDescriptionSet[Index].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[Index].offset = offsetof(SSimplePointData, Pos);
        Index++;

        AttributeDescriptionSet[Index].binding = 0;
        AttributeDescriptionSet[Index].location = Index;
        AttributeDescriptionSet[Index].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[Index].offset = offsetof(SSimplePointData, Normal);
        Index++;

        AttributeDescriptionSet[Index].binding = 0;
        AttributeDescriptionSet[Index].location = Index;
        AttributeDescriptionSet[Index].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptionSet[Index].offset = offsetof(SSimplePointData, TexCoord);
        Index++;

        AttributeDescriptionSet[Index].binding = 0;
        AttributeDescriptionSet[Index].location = Index;
        AttributeDescriptionSet[Index].format = VK_FORMAT_R32_UINT;
        AttributeDescriptionSet[Index].offset = offsetof(SSimplePointData, TexIndex);
        Index++;

        return AttributeDescriptionSet;
    }
};
