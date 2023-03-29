#pragma once
#include "PipelineVisualizePrimitive.h"

class CPipelineLine : public CPipelineVisualizePrimitive
{
public:
    void add(const Visualize::Line& vLine)
    {
        m_Lines.emplace_back(vLine);
        __updateVertexBuffer();
    }

    void clear()
    {
        m_Lines.clear();
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

        m_VertexNum = m_Lines.size() * 2;
        if (m_VertexNum > 0)
        {
            VkDeviceSize BufferSize = sizeof(CPipelineVisualizePrimitive::SPointData) * m_VertexNum;

            std::vector<CPipelineVisualizePrimitive::SPointData> Vertices;
            for (const auto& Line : m_Lines)
            {
                glm::vec3 N = glm::vec3(0.0, 1.0, 0.0);

                Vertices.push_back({ Line.Start, N });
                Vertices.push_back({ Line.End, N });
            }
            _ASSERTE(BufferSize == Vertices.size() * sizeof(CPipelineVisualizePrimitive::SPointData));

            m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            m_VertexBuffer.stageFill(Vertices.data(), BufferSize);
        }
    }

    std::vector<Visualize::Line> m_Lines;
};