#pragma once
#include "Pipeline.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "VertexAttributeDescriptor.h"
#include "VisualizePrimitive.h"

#include <glm/glm.hpp>

class CPipelineVisualizePrimitive : public IPipeline
{
public:
    struct SPointData
    {
        glm::vec3 Pos;
        glm::vec3 Normal;
        glm::vec3 Color;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
            Descriptor.add(_GET_ATTRIBUTE_INFO(Color));
            return Descriptor.generate();
        }
    };

    void updateUniformBuffer(CCamera::CPtr vCamera);
    virtual void recordCommandV(CCommandBuffer::Ptr vCommandBuffer);

protected:
    virtual void _createV() override;
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _destroyV() override;
    
    uint32_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;

private:
    void __updateDescriptorSet();
    
    vk::CUniformBuffer::Ptr m_pVertUniformBuffer;
    vk::CUniformBuffer::Ptr m_pFragUniformBuffer;
};