#pragma once
#include "Pipeline.h"
#include "VertexAttributeDescriptor.h"
#include "Camera.h"

#include <glm/glm.hpp>

class CPipelineVisualizePrimitiveInstanced : public IPipeline
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
    };

    void add(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor = glm::vec3(1.0, 1.0, 1.0));
    void clear();
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initPushConstantV(VkCommandBuffer vCommandBuffer) override;
    virtual void _destroyV() override;

    virtual std::vector<SPointData> _createPrimitive() = 0;

private:
    void __updateDescriptorSet();
    void __updateVertexBuffer();

    std::vector<glm::vec3> m_PrimitiveCenterSet;
    std::vector<glm::vec3> m_PrimitiveScaleSet;
    std::vector<glm::vec3> m_ColorSet;

    size_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
};
