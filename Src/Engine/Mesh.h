#pragma once
#include "Pointer.h"
#include "Common.h"
#include "GeneralDataArray.h"
#include "Transform.h"
#include "BoundingBox.h"

#include <string>
#include <glm/glm.hpp>

enum class E3DObjectPrimitiveType
{
    TRIAGNLE_LIST,
    TRIAGNLE_STRIP_LIST,
    INDEXED_TRIAGNLE_LIST,
};

#define _DEFINE_MESH_DATA_ARRAY_PROPERTY(Name, Type) public: _DEFINE_GETTER_SETTER_POINTER(Name, IDataArray<##Type##>::Ptr) private: IDataArray<##Type##>::Ptr m_p##Name = make<CGeneralDataArray<Type>>();

class CMeshData
{
public:
    _DEFINE_PTR(CMeshData);

    CMeshData copyDeeply();
    
    _DEFINE_GETTER_SETTER(PrimitiveType, E3DObjectPrimitiveType)

    static const uint32_t InvalidLightmapIndex;
    bool hasLightmap() const
    {
        return !m_pLightmapTexCoordArray->empty();
    }
        
    SAABB getAABB() const
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

private:
    E3DObjectPrimitiveType m_PrimitiveType = E3DObjectPrimitiveType::TRIAGNLE_LIST;

    _DEFINE_MESH_DATA_ARRAY_PROPERTY(VertexArray, glm::vec3)
    _DEFINE_MESH_DATA_ARRAY_PROPERTY(NormalArray, glm::vec3)
    _DEFINE_MESH_DATA_ARRAY_PROPERTY(ColorArray, glm::vec3)
    _DEFINE_MESH_DATA_ARRAY_PROPERTY(TexIndexArray, uint32_t)
    _DEFINE_MESH_DATA_ARRAY_PROPERTY(TexCoordArray, glm::vec2)
    _DEFINE_MESH_DATA_ARRAY_PROPERTY(LightmapIndexArray, uint32_t)
    _DEFINE_MESH_DATA_ARRAY_PROPERTY(LightmapTexCoordArray, glm::vec2)
};

class CMesh
{
public:
	_DEFINE_PTR(CMesh);
    virtual ~CMesh() = default;

    _DEFINE_GETTER_SETTER(Name, std::string)

    virtual CMeshData getMeshDataV() const = 0; // FIXME: use pointer or not? which mean will the data be updated after returned?

    SAABB getAABB() const
    {
        // FIXME: bad design, how to update AABB?
        return getMeshDataV().getAABB();
    }

private:
	std::string m_Name = "Default Mesh";
};

class CMeshTriangleList : public CMesh
{
public:
    _DEFINE_PTR(CMeshTriangleList);

    virtual CMeshData getMeshDataV() const override
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

    void setMeshData(CMeshData vData)
    {
        m_Data = std::move(vData);
    }

private:
    CMeshData m_Data;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicQuad : public CMesh
{
public:
    _DEFINE_PTR(CMeshBasicQuad);

    static CMeshData MeshData;

    virtual CMeshData getMeshDataV() const override;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicCube : public CMesh
{
public:
    _DEFINE_PTR(CMeshBasicCube);

    static CMeshData MeshData;

    virtual CMeshData getMeshDataV() const override;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicSphere : public CMesh
{
public:
    _DEFINE_PTR(CMeshBasicCube);

    static CMeshData MeshData;

    virtual CMeshData getMeshDataV() const override;
};

namespace Mesh
{
    CMeshTriangleList::Ptr bakeTransform(CMesh::CPtr vMesh, cptr<CTransform> vTransform);
}
