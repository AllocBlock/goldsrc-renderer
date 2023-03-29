#pragma once
#include "PipelineVisualizePrimitive.h"

class CPipelineTriangle : public CPipelineVisualizePrimitive
{
public:
    void add(const Visualize::Triangle& vTriangle)
    {
        glm::vec3 Normal = glm::normalize(glm::cross(vTriangle.B - vTriangle.A, vTriangle.C - vTriangle.A));
        m_Triangles.emplace_back(std::make_pair(vTriangle, Normal));
        __updateVertexBuffer();
    }

    void clear()
    {
        m_Triangles.clear();
        __updateVertexBuffer();
    }

protected:
    virtual CPipelineDescriptor _getPipelineDescriptionV() override
    {
        CPipelineDescriptor Descriptor = CPipelineVisualizePrimitive::_getPipelineDescriptionV();
        Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

        return Descriptor;
    }

private:
    void __updateVertexBuffer()
    {
        m_pDevice->waitUntilIdle();
        m_VertexBuffer.destroy();

        m_VertexNum = m_Triangles.size() * 3;
        if (m_VertexNum > 0)
        {
            VkDeviceSize BufferSize = sizeof(CPipelineVisualizePrimitive::SPointData) * m_VertexNum;

            std::vector<CPipelineVisualizePrimitive::SPointData> Vertices;
            for (const auto& Pair : m_Triangles)
            {
                const auto& Tri = Pair.first;
                const auto& N = Pair.second;

                Vertices.push_back({ Tri.A, N });
                Vertices.push_back({ Tri.B, N });
                Vertices.push_back({ Tri.C, N });
            }
            _ASSERTE(BufferSize == Vertices.size() * sizeof(CPipelineVisualizePrimitive::SPointData));

            m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            m_VertexBuffer.stageFill(Vertices.data(), BufferSize);
        }
    }


    std::vector<std::pair<Visualize::Triangle, glm::vec3>> m_Triangles;
};