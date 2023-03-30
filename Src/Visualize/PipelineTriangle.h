#pragma once
#include "VisualizePrimitive.h"
#include "PipelineVisualizePrimitiveTyped.h"

class CPipelineTriangle : public CPipelineVisualizePrimitiveTyped<Visualize::Triangle>
{
protected:
    virtual void _onAddV(const Visualize::Triangle& vTriangle) override
    {
        glm::vec3 Normal = glm::normalize(glm::cross(vTriangle.B - vTriangle.A, vTriangle.C - vTriangle.A));
        m_Normals.emplace_back(Normal);
    }

    virtual void _onClearV() override
    {
        m_Normals.clear();
    }
    
    virtual CPipelineDescriptor _getPipelineDescriptionV() override
    {
        CPipelineDescriptor Descriptor = CPipelineVisualizePrimitiveTyped::_getPipelineDescriptionV();

        Descriptor.setVertShaderPath("shaders/triangleShader.vert");
        Descriptor.setFragShaderPath("shaders/triangleShader.frag");

        Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

        return Descriptor;
    }
    
    virtual std::vector<CPipelineVisualizePrimitive::SPointData> _convertPrimitive2PointData(size_t vIndex, const Visualize::Triangle& vPrimitive, const glm::vec3& vColor) override
    {
        const glm::vec3& N = m_Normals[vIndex / 3];
        return  {
            {vPrimitive.A, N, vColor},
            {vPrimitive.B, N, vColor},
            {vPrimitive.C, N, vColor},
        };
    }

private:
    std::vector<glm::vec3> m_Normals; // one triangle (3 point) map to one normal
};