#pragma once
#include "VertexAttributeDescriptor.h"
#include "Mesh.h"

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
    
    static std::vector<SGoldSrcPointData> extractFromMeshData(const CMeshData& vMeshData)
    {
        auto pVertexArray = vMeshData.getVertexArray();
        auto pColorArray = vMeshData.getColorArray();
        auto pNormalArray = vMeshData.getNormalArray();
        auto pTexCoordArray = vMeshData.getTexCoordArray();
        auto pLightmapCoordArray = vMeshData.getLightmapTexCoordArray();
        auto pTexIndexArray = vMeshData.getTexIndexArray();

        size_t NumPoint = pVertexArray->size();
        _ASSERTE(NumPoint == pColorArray->size());
        _ASSERTE(NumPoint == pNormalArray->size());
        _ASSERTE(NumPoint == pTexCoordArray->size());

        bool HasLightmap = vMeshData.hasLightmap();
        if (HasLightmap)
        {
            _ASSERTE(NumPoint == pLightmapCoordArray->size());
            _ASSERTE(NumPoint == pTexIndexArray->size());
        }

        std::vector<SGoldSrcPointData> PointData(NumPoint);
        for (size_t i = 0; i < NumPoint; ++i)
        {
            PointData[i].Pos = pVertexArray->get(i);
            PointData[i].Color = pColorArray->get(i);
            PointData[i].Normal = pNormalArray->get(i);
            PointData[i].TexCoord = pTexCoordArray->get(i);
            PointData[i].LightmapCoord = HasLightmap ? pLightmapCoordArray->get(i) : glm::vec2(0.0, 0.0);
            PointData[i].TexIndex = pTexIndexArray->get(i);
        }
        return PointData;
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

    static std::vector<SSimplePointData> extractFromMeshData(const CMeshData& vMeshData)
    {
        auto pVertexArray = vMeshData.getVertexArray();
        auto pNormalArray = vMeshData.getNormalArray();
        auto pTexCoordArray = vMeshData.getTexCoordArray();
        auto pTexIndexArray = vMeshData.getTexIndexArray();

        size_t NumPoint = pVertexArray->size();
        _ASSERTE(NumPoint == pNormalArray->size());
        _ASSERTE(NumPoint == pTexCoordArray->size());
        _ASSERTE(NumPoint == pTexIndexArray->size());

        std::vector<SSimplePointData> PointData(NumPoint);
        for (size_t i = 0; i < NumPoint; ++i)
        {
            PointData[i].Pos = pVertexArray->get(i);
            PointData[i].Normal = pNormalArray->get(i);
            PointData[i].TexCoord = pTexCoordArray->get(i);
            PointData[i].TexIndex = pTexIndexArray->get(i);
        }
        return PointData;
    }
};
