#pragma once
#include "PipelineVisualizePrimitive.h"

template <typename Primitive_t>
class CPipelineVisualizePrimitiveTyped : public CPipelineVisualizePrimitive
{
public:
    void add(const Primitive_t& vPrimitive, const glm::vec3& vColor = glm::vec3(1.0, 1.0, 1.0))
    {
        m_Primitives.emplace_back(vPrimitive);
        m_Colors.emplace_back(vColor);
        m_NeedUpdateVertexBuffer = true;
        _onAddV(vPrimitive);
    }

    void clear()
    {
        m_Primitives.clear();
        m_Colors.clear();
        m_NeedUpdateVertexBuffer = true;
        _onClearV();
    }

    virtual void recordCommandV(CCommandBuffer::Ptr vCommandBuffer) override
    {
        if (m_NeedUpdateVertexBuffer)
        {
            m_NeedUpdateVertexBuffer = false;
            __updateVertexBuffer();
        }

        CPipelineVisualizePrimitive::recordCommandV(vCommandBuffer);
    }

protected:
    virtual void _onAddV(const Primitive_t& vPrimitive) {}
    virtual void _onClearV() {}
    virtual std::vector<CPipelineVisualizePrimitive::SPointData> _convertPrimitive2PointData(size_t vIndex, const Primitive_t& vPrimitive, const glm::vec3& vColor) = 0;

private:
    void __updateVertexBuffer()
    {
        m_pDevice->waitUntilIdle();
        m_VertexBuffer.destroy();

        if (!m_Primitives.empty())
        {
            std::vector<CPipelineVisualizePrimitive::SPointData> Vertices;
            for (size_t i = 0; i < m_Primitives.size(); ++i)
            {
                const auto& PointDatas = _convertPrimitive2PointData(i, m_Primitives[i], m_Colors[i]);
                Vertices.insert(Vertices.end(), PointDatas.begin(), PointDatas.end());
            }
            m_VertexNum = static_cast<uint32_t>(Vertices.size());

            VkDeviceSize BufferSize = sizeof(CPipelineVisualizePrimitive::SPointData) * m_VertexNum;
            m_VertexBuffer.create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            m_VertexBuffer.stageFill(Vertices.data(), BufferSize);
        }
        else
            m_VertexNum = 0;
    }

    bool m_NeedUpdateVertexBuffer = false;
    std::vector<Primitive_t> m_Primitives;
    std::vector<glm::vec3> m_Colors;
};