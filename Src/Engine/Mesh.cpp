#include "Mesh.h"

#include "BasicMesh.h"

const uint32_t CMeshData::InvalidLightmapIndex = std::numeric_limits<uint32_t>::max();

CMeshData __convertBasicMeshToMeshData(const std::vector<BasicMesh::SVertex>& vBasicMesh)
{
    auto pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    auto pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    for (const auto& Vertex : vBasicMesh)
    {
        pVertexArray->append(Vertex.Pos);
        pNormalArray->append(Vertex.Normal);
    } 

    auto MeshData = CMeshData();
    MeshData.setVertexArray(pVertexArray);
    MeshData.setNormalArray(pNormalArray);
    return MeshData;
}

CMeshData CMeshBasicQuad::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitQuadFaceSet());
CMeshData CMeshBasicCube::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitCubeFaceSet());
CMeshData CMeshBasicSphere::MeshData = __convertBasicMeshToMeshData(BasicMesh::getUnitSphereFaceSet());

CMeshData CMeshData::copyDeeply()
{
    CMeshData NewData;
    NewData.m_PrimitiveType = m_PrimitiveType;
    NewData.m_pVertexArray = m_pVertexArray->copy();
    NewData.m_pNormalArray = m_pNormalArray->copy();
    NewData.m_pColorArray = m_pColorArray->copy();
    NewData.m_pTexCoordArray = m_pTexCoordArray->copy();
    NewData.m_pTexIndexArray = m_pTexIndexArray->copy();
    NewData.m_pLightmapTexCoordArray = m_pLightmapTexCoordArray->copy();

    return NewData;
}

bool CMeshData::hasLightmap() const
{
    return !m_pLightmapTexCoordArray->empty();
}

SAABB CMeshData::getAABB() const
{
    if (m_pVertexArray->empty()) return SAABB::InvalidAABB;

    glm::vec3 Min(INFINITY), Max(-INFINITY);
    // FIXME: bad design, calculation for every call
    for (size_t i = 0; i < m_pVertexArray->size(); ++i)
    {
        const auto& Pos = m_pVertexArray->get(i);
        Min.x = glm::min(Min.x, Pos.x);
        Min.y = glm::min(Min.y, Pos.y);
        Min.z = glm::min(Min.z, Pos.z);
        Max.x = glm::max(Max.x, Pos.x);
        Max.y = glm::max(Max.y, Pos.y);
        Max.z = glm::max(Max.z, Pos.z);
    }
    return SAABB(Min, Max);
}

SAABB CMesh::getAABB() const
{
    // FIXME: bad design, how to update AABB?
    return getMeshDataV().getAABB();
}

CMeshData CMeshTriangleList::getMeshDataV() const
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

void CMeshTriangleList::setMeshData(CMeshData vData)
{
    m_Data = std::move(vData);
}

CMeshData CMeshBasicQuad::getMeshDataV() const
{
    return CMeshBasicQuad::MeshData;
}

CMeshData CMeshBasicCube::getMeshDataV() const
{
    return CMeshBasicCube::MeshData;
}

CMeshData CMeshBasicSphere::getMeshDataV() const
{
    return CMeshBasicSphere::MeshData;
}

sptr<CMeshTriangleList> Mesh::bakeTransform(cptr<CMesh> vMesh, cptr<CTransform> vTransform)
{
    auto pMesh = make<CMeshTriangleList>();
    CMeshData Data = vMesh->getMeshDataV().copyDeeply();

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
