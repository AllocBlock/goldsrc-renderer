#include "Mesh.h"

#include "BasicMesh.h"

CGeneralMeshDataTest __convertBasicMeshToMeshData(const std::vector<BasicMesh::SVertex>& vBasicMesh)
{
    auto pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    auto pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    for (const auto& Vertex : vBasicMesh)
    {
        pVertexArray->append(Vertex.Pos);
        pNormalArray->append(Vertex.Normal);
    } 

    auto MeshData = CGeneralMeshDataTest();
    MeshData.setVertexArray(pVertexArray);
    MeshData.setNormalArray(pNormalArray);
    return MeshData;
}

CGeneralMeshDataTest CMeshBasicQuad::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitQuadFaceSet());
CGeneralMeshDataTest CMeshBasicCube::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitCubeFaceSet());
CGeneralMeshDataTest CMeshBasicSphere::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitSphereFaceSet());

CGeneralMeshDataTest CMeshTriangleList::getMeshData() const
{
    return m_Data;
}

void CMeshTriangleList::addTriangles(std::vector<glm::vec3> vPosSet, std::vector<glm::vec3> vNormalSet)
{
    size_t VertexNum = vPosSet.size();
    _ASSERTE(VertexNum % 3 == 0);
    _ASSERTE(VertexNum == vNormalSet.size());
        
    auto pVertexArray = m_Data.getVertexArray();
    auto pNormalArray = m_Data.getNormalArray();
    for (size_t i = 0; i < VertexNum; ++i)
    {
        pVertexArray->append(vPosSet[i]);
        pNormalArray->append(vNormalSet[i]);
    }
}

CGeneralMeshDataTest CMeshBasicQuad::getMeshData() const
{
    return CMeshBasicQuad::MeshData;
}

CGeneralMeshDataTest CMeshBasicCube::getMeshData() const
{
    return CMeshBasicCube::MeshData;
}

CGeneralMeshDataTest CMeshBasicSphere::getMeshData() const
{
    return CMeshBasicSphere::MeshData;
}
