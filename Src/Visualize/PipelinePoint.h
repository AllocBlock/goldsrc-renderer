#pragma once
#include "PipelineVisualizePrimitive.h"

class CPipelinePoint : public CPipelineVisualizePrimitive
{
public:
    void add(const Visualize::Point& vPoint)
    {
        m_Points.emplace_back(vPoint);
        __updateVertexBuffer();
    }

    void clear()
    {
        m_Points.clear();
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

        m_VertexNum = m_Points.size();
        if (m_VertexNum > 0)
        {
            VkDeviceSize BufferSize = sizeof(CPipelineVisualizePrimitive::SPointData) * m_VertexNum;

            std::vector<CPipelineVisualizePrimitive::SPointData> Vertices;
            for (const auto& Point : m_Points)
            {
                glm::vec3 N = glm::vec3(0.0, 1.0, 0.0);

                Vertices.push_back({ Point, N });
            }
            _ASSERTE(BufferSize == Vertices.size() * sizeof(CPipelineVisualizePrimitive::SPointData));

            m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            m_VertexBuffer.stageFill(Vertices.data(), BufferSize);
        }
    }


    std::vector<Visualize::Point> m_Points;
};