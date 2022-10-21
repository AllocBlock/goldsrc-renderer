#pragma once
#include "Pipeline.h"
#include "UniformBuffer.h"
#include "VertexAttributeDescriptor.h"
#include "Mesh.h"

#include <glm/glm.hpp>

class CPipelineShadowMap : public IPipeline
{
public:
    struct SPointData
    {
        glm::vec3 Pos;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            return Descriptor.generate();
        }

        static std::vector<SPointData> extractFromMeshData(const CGeneralMeshDataTest& vMeshData)
        {
            auto pVertexArray = vMeshData.getVertexArray();

            size_t NumPoint = pVertexArray->size();

            std::vector<SPointData> PointData(NumPoint);
            for (size_t i = 0; i < NumPoint; ++i)
            {
                PointData[i].Pos = pVertexArray->get(i);
            }
            return PointData;
        }
    };

    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vLightViewProj, float vLightNear, float vLightFar);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __destroyResources();

    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
};

