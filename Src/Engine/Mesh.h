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

class CMeshDataGeneral
{
public:
    _DEFINE_PTR(CMeshDataGeneral);

    bool getVisibleState() { return m_IsVisible; }
    void setVisibleState(bool vState) { m_IsVisible = vState; }
    CMeshDataGeneral copy();

    _DEFINE_GETTER_SETTER_POINTER(VertexArray, IDataArray<glm::vec3>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(NormalArray, IDataArray<glm::vec3>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(ColorArray, IDataArray<glm::vec3>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(TexCoordArray, IDataArray<glm::vec2>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(TexIndexArray, IDataArray<uint32_t>::Ptr)
    _DEFINE_GETTER_SETTER(PrimitiveType, E3DObjectPrimitiveType)
        
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

protected:
    bool m_IsVisible = true;
    E3DObjectPrimitiveType m_PrimitiveType = E3DObjectPrimitiveType::TRIAGNLE_LIST;
    IDataArray<glm::vec3>::Ptr m_pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec3>::Ptr m_pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec3>::Ptr m_pColorArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec2>::Ptr m_pTexCoordArray = make<CGeneralDataArray<glm::vec2>>();
    IDataArray<uint32_t>::Ptr m_pTexIndexArray = make<CGeneralDataArray<glm::uint32_t>>();
};

template <typename MeshData_t = CMeshDataGeneral>
class CMesh
{
public:
	_DEFINE_PTR(CMesh);
    virtual ~CMesh() = default;

    _DEFINE_GETTER_SETTER(Name, std::string)

    virtual MeshData_t getMeshData() const = 0; // FIXME: use pointer or not? which mean will the data be updated after returned?

    SAABB getAABB() const
    {
        // FIXME: bad design, how to update AABB?
        return getMeshData().getAABB();
    }

private:
	std::string m_Name = "Default Mesh";
};

template <typename MeshData_t = CMeshDataGeneral>
class CMeshTriangleList : public CMesh<MeshData_t>
{
public:
    _DEFINE_PTR(CMeshTriangleList);

    virtual MeshData_t getMeshData() const override
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

    void setMeshData(MeshData_t vData)
    {
        m_Data = std::move(vData);
    }

private:
    MeshData_t m_Data;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicQuad : public CMesh<CMeshDataGeneral>
{
public:
    _DEFINE_PTR(CMeshBasicQuad);

    static CMeshDataGeneral MeshData;

    virtual CMeshDataGeneral getMeshData() const override;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicCube : public CMesh<CMeshDataGeneral>
{
public:
    _DEFINE_PTR(CMeshBasicCube);

    static CMeshDataGeneral MeshData;

    virtual CMeshDataGeneral getMeshData() const override;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicSphere : public CMesh<CMeshDataGeneral>
{
public:
    _DEFINE_PTR(CMeshBasicCube);

    static CMeshDataGeneral MeshData;

    virtual CMeshDataGeneral getMeshData() const override;
};

namespace Mesh
{
    CMeshTriangleList<CMeshDataGeneral>::Ptr bakeTransform(CMesh<CMeshDataGeneral>::CPtr vMesh, cptr<STransform> vTransform);
}
