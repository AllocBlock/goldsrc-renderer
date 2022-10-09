#pragma once
#include "Pointer.h"
#include "Common.h"
#include "GeneralDataArray.h"

#include <string>
#include <array>
#include <glm/glm.hpp>

enum class E3DObjectPrimitiveType
{
    TRIAGNLE_LIST,
    TRIAGNLE_STRIP_LIST,
    INDEXED_TRIAGNLE_LIST,
};

class CGeneralMeshDataTest
{
public:
    _DEFINE_PTR(CGeneralMeshDataTest);

    bool getVisibleState() { return m_IsVisible; }
    void setVisibleState(bool vState) { m_IsVisible = vState; }

    _DEFINE_GETTER_SETTER_POINTER(VertexArray, IDataArray<glm::vec3>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(NormalArray, IDataArray<glm::vec3>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(ColorArray, IDataArray<glm::vec3>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(TexCoordArray, IDataArray<glm::vec2>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(TexIndexArray, IDataArray<uint32_t>::Ptr)
    _DEFINE_GETTER_SETTER(PrimitiveType, E3DObjectPrimitiveType)

protected:
    bool m_IsVisible = true;
    E3DObjectPrimitiveType m_PrimitiveType = E3DObjectPrimitiveType::TRIAGNLE_LIST;
    IDataArray<glm::vec3>::Ptr m_pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec3>::Ptr m_pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec3>::Ptr m_pColorArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec2>::Ptr m_pTexCoordArray = make<CGeneralDataArray<glm::vec2>>();
    IDataArray<uint32_t>::Ptr m_pTexIndexArray = make<CGeneralDataArray<glm::uint32_t>>();
};

class CMesh
{
public:
	_DEFINE_PTR(CMesh);
    _DEFINE_GETTER_SETTER(Name, std::string)

    virtual CGeneralMeshDataTest getMeshData() const = 0; // FIXME: use pointer or not? which mean will the data be updated after returned?

private:
	std::string m_Name = "Default Mesh";
};

class CMeshTriangleList : public CMesh
{
public:
    _DEFINE_PTR(CMeshTriangleList);

    virtual CGeneralMeshDataTest getMeshData() const override
    {
        return m_Data;
    }

    void addTriangles(std::vector<glm::vec3> vPosSet, std::vector<glm::vec3> vNormalSet)
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

private:
    CGeneralMeshDataTest m_Data;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicCube : public CMesh
{
public:
    _DEFINE_PTR(CMeshBasicCube);

    CMeshBasicCube(glm::vec3 vCenter = glm::vec3(0.0f), float vSize = 1.0f):
        m_Center(vCenter), m_Size(vSize)
    {}

    virtual CGeneralMeshDataTest getMeshData() const override
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

        for (auto& Vertex : VertexSet)
            Vertex = m_Center + Vertex * m_Size * 0.5f;

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

private:
    glm::vec3 m_Center = glm::vec3(0.0f);
    float m_Size = 1.0f;
};