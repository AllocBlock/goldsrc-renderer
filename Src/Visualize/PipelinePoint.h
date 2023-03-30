#pragma once
#include "VisualizePrimitive.h"
#include "PipelineVisualizePrimitiveTyped.h"

class CPipelinePoint : public CPipelineVisualizePrimitiveTyped<Visualize::Point>
{
protected:
    virtual CPipelineDescriptor _getPipelineDescriptionV() override
    {
        CPipelineDescriptor Descriptor = CPipelineVisualizePrimitiveTyped::_getPipelineDescriptionV();

        Descriptor.setVertShaderPath("shaders/pointShader.vert");
        Descriptor.setFragShaderPath("shaders/pointShader.frag");

        Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, false);

        return Descriptor;
    }

    virtual std::vector<CPipelineVisualizePrimitive::SPointData> _convertPrimitive2PointData(size_t vIndex, const Visualize::Point& vPrimitive, const glm::vec3& vColor) override
    {
        glm::vec3 N = glm::vec3(0.0, 1.0, 0.0);
        return  {
            {vPrimitive, N, vColor},
        };
    }
};