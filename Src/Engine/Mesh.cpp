#include "Mesh.h"

#include "BasicMesh.h"

CMeshDataGeneral __convertBasicMeshToMeshData(const std::vector<BasicMesh::SVertex>& vBasicMesh)
{
    auto pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    auto pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    for (const auto& Vertex : vBasicMesh)
    {
        pVertexArray->append(Vertex.Pos);
        pNormalArray->append(Vertex.Normal);
    } 

    auto MeshData = CMeshDataGeneral();
    MeshData.setVertexArray(pVertexArray);
    MeshData.setNormalArray(pNormalArray);
    return MeshData;
}

CMeshDataGeneral CMeshBasicQuad::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitQuadFaceSet());
CMeshDataGeneral CMeshBasicCube::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitCubeFaceSet());
CMeshDataGeneral CMeshBasicSphere::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitSphereFaceSet());

CMeshDataGeneral CMeshDataGeneral::copy()
{
    CMeshDataGeneral NewData;
    NewData.m_PrimitiveType = m_PrimitiveType;
    NewData.m_pVertexArray = m_pVertexArray->copy();
    NewData.m_pNormalArray = m_pNormalArray->copy();
    NewData.m_pColorArray = m_pColorArray->copy();
    NewData.m_pTexCoordArray = m_pTexCoordArray->copy();
    NewData.m_pTexIndexArray = m_pTexIndexArray->copy();

    return NewData;
}

CMeshDataGeneral CMeshBasicQuad::getMeshData() const
{
    return CMeshBasicQuad::MeshData;
}

CMeshDataGeneral CMeshBasicCube::getMeshData() const
{
    return CMeshBasicCube::MeshData;
}

CMeshDataGeneral CMeshBasicSphere::getMeshData() const
{
    return CMeshBasicSphere::MeshData;
}

CMeshTriangleList<CMeshDataGeneral>::Ptr Mesh::bakeTransform(CMesh<CMeshDataGeneral>::CPtr vMesh, cptr<STransform> vTransform)
{
    auto pMesh = make<CMeshTriangleList<CMeshDataGeneral>>();
    CMeshDataGeneral Data = vMesh->getMeshData().copy();

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
