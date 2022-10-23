#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "VertexAttributeDescriptor.h"
#include "Camera.h"
#include "Mesh.h"

#include <glm/glm.hpp>

class CPipelineShade : public IPipeline
{
public:
    struct SPointData
    {
        glm::vec3 Pos;
        glm::vec3 Normal;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
            return Descriptor.generate();
        }

        static std::vector<SPointData> extractFromMeshData(const CMeshDataGeneral& vMeshData)
        {
            auto pVertexArray = vMeshData.getVertexArray();
            auto pNormalArray = vMeshData.getNormalArray();

            size_t NumPoint = pVertexArray->size();
            _ASSERTE(NumPoint == pNormalArray->size());

            std::vector<CPipelineShade::SPointData> PointData(NumPoint);
            for (size_t i = 0; i < NumPoint; ++i)
            {
                PointData[i].Pos = pVertexArray->get(i);
                PointData[i].Normal = pNormalArray->get(i);
            }
            return PointData;
        }
    };

    void setShadowMapImageViews(std::vector<VkImageView> vShadowMapImageViews);
    bool isShadowMapReady() { return !m_ShadowMapImageViewSet.empty(); }
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera, CCamera::CPtr vLightCamera, uint32_t vShadowMapSize);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __destroyResources();

    vk::CSampler m_Sampler;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CImage m_PlaceholderImage;
    std::vector<VkImageView> m_ShadowMapImageViewSet;
};

