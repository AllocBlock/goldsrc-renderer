#include "Mesh.h"

#include "BasicMesh.h"

CGeneralMeshDataTest __createCube()
{
    /*
    *   4------5      y
    *  /|     /|      |
    * 0------1 |      |
    * | 7----|-6      -----x
    * |/     |/      /
    * 3------2      z
    */
    std::array<glm::vec3, 8> VertexSet =
    {
        glm::vec3(-1,  1,  1),
        glm::vec3(1,  1,  1),
        glm::vec3(1, -1,  1),
        glm::vec3(-1, -1,  1),
        glm::vec3(-1,  1, -1),
        glm::vec3(1,  1, -1),
        glm::vec3(1, -1, -1),
        glm::vec3(-1, -1, -1),
    };

    const std::array<size_t, 36> IndexSet =
    {
        0, 1, 2, 0, 2, 3, // front
        5, 4, 7, 5, 7, 6, // back
        4, 5, 1, 4, 1, 0, // up
        3, 2, 6, 3, 6, 7, // down
        4, 0, 3, 4, 3, 7, // left
        1, 5, 6, 1, 6, 2  // right
    };

    std::array<glm::vec3, 6> NormalSet =
    {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(1, 0, 0),
    };

    auto pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    auto pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    for (size_t i = 0; i < IndexSet.size(); ++i)
    {
        size_t Index = IndexSet[i];
        pVertexArray->append(VertexSet[Index]);
        pNormalArray->append(NormalSet[i / 6]);
    }

    auto MeshData = CGeneralMeshDataTest();
    MeshData.setVertexArray(pVertexArray);
    MeshData.setNormalArray(pNormalArray);
    return MeshData;
}

CGeneralMeshDataTest __createSphere()
{
    auto pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    auto pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    for (const auto& Vertex : BasicMesh::getUnitSphereFaceSet())
    {
        pVertexArray->append(Vertex.Pos);
        pNormalArray->append(Vertex.Normal);
    }

    auto MeshData = CGeneralMeshDataTest();
    MeshData.setVertexArray(pVertexArray);
    MeshData.setNormalArray(pNormalArray);
    return MeshData;
}

CGeneralMeshDataTest CMeshBasicCube::MeshData = __createCube();
CGeneralMeshDataTest CMeshBasicSphere::MeshData = __createSphere();

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

CGeneralMeshDataTest CMeshBasicCube::getMeshData() const
{
    return CMeshBasicCube::MeshData;
}

CGeneralMeshDataTest CMeshBasicSphere::getMeshData() const
{
    return CMeshBasicSphere::MeshData;
}
