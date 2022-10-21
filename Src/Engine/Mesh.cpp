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

CGeneralMeshDataTest CGeneralMeshDataTest::copy()
{
    CGeneralMeshDataTest NewData;
    NewData.m_PrimitiveType = m_PrimitiveType;
    NewData.m_pVertexArray = m_pVertexArray->copy();
    NewData.m_pNormalArray = m_pNormalArray->copy();
    NewData.m_pColorArray = m_pColorArray->copy();
    NewData.m_pTexCoordArray = m_pTexCoordArray->copy();
    NewData.m_pTexIndexArray = m_pTexIndexArray->copy();

    return NewData;
}

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

CMeshTriangleList::Ptr Mesh::bakeTransform(CMesh::CPtr vMesh, cptr<STransform> vTransform)
{
    auto pMesh = make<CMeshTriangleList>();
    CGeneralMeshDataTest Data = vMesh->getMeshData().copy();

    auto pVertArray = Data.getVertexArray();
    auto pNormalArray = Data.getNormalArray();
    _ASSERTE(pVertArray->size() == pNormalArray->size() || pNormalArray->size() == 0);

    glm::mat3 NormalMat = glm::transpose(glm::inverse(glm::mat3(vTransform->getAbsoluteModelMat4())));
    for (size_t i = 0; i < pVertArray->size(); ++i)
    {
        pVertArray->set(i, vTransform->applyAbsoluteOnPoint(pVertArray->get(i)));
        if (!pNormalArray->empty())
        {
            pNormalArray->set(i, NormalMat * pNormalArray->get(i));
        }
    }
    pMesh->setMeshData(std::move(Data));
    return pMesh;
}
