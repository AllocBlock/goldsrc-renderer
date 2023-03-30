#pragma once
#include "VisualizePrimitive.h"
#include "PipelineVisualizePrimitiveTyped.h"

class CPipelineLine : public CPipelineVisualizePrimitiveTyped<Visualize::Line>
{
protected:
    virtual CPipelineDescriptor _getPipelineDescriptionV() override
    {
        CPipelineDescriptor Descriptor = CPipelineVisualizePrimitiveTyped::_getPipelineDescriptionV();

        Descriptor.setVertShaderPath("shaders/lineShader.vert");
        Descriptor.setFragShaderPath("shaders/lineShader.frag");

        Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false);

        return Descriptor;
    }

    virtual std::vector<CPipelineVisualizePrimitive::SPointData> _convertPrimitive2PointData(size_t vIndex, const Visualize::Line& vPrimitive, const glm::vec3& vColor) override
    {
        glm::vec3 N = glm::vec3(0.0, 1.0, 0.0);
        return  {
            {vPrimitive.Start, N, vColor},
            {vPrimitive.End, N, vColor},
        };
    }
};