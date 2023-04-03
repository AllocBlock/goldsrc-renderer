#pragma once
#include "Pipeline.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "VertexAttributeDescriptor.h"
#include "VisualizePrimitive.h"
#include "Environment.h"

#include <map>
#include <filesystem>
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

    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    virtual void recordCommandV(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;
    
    size_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;

private:
    void __updateDescriptorSet();
    
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
};