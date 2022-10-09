#include "3DObject.h"

std::optional<Math::S3DBoundingBox> CGeneralMeshData::getBoundingBox() const
{
    std::optional<Math::S3DBoundingBox> CachedBoundingBox = std::nullopt;
    if (CachedBoundingBox.has_value()) return CachedBoundingBox.value();
    if (m_pVertexArray->empty()) return std::nullopt;

    Math::S3DBoundingBox BoundingBox;
    BoundingBox.Min = glm::vec3(INFINITY, INFINITY, INFINITY);
    BoundingBox.Max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);
    for (size_t i = 0; i < m_pVertexArray->size(); ++i)
    {
        auto Vertex = m_pVertexArray->get(i);
        BoundingBox.Min.x = std::min<float>(BoundingBox.Min.x, Vertex.x);
        BoundingBox.Min.y = std::min<float>(BoundingBox.Min.y, Vertex.y);
        BoundingBox.Min.z = std::min<float>(BoundingBox.Min.z, Vertex.z);
        BoundingBox.Max.x = std::max<float>(BoundingBox.Max.x, Vertex.x);
        BoundingBox.Max.y = std::max<float>(BoundingBox.Max.y, Vertex.y);
        BoundingBox.Max.z = std::max<float>(BoundingBox.Max.z, Vertex.z);
    }
    CachedBoundingBox = BoundingBox;
    return BoundingBox;
}