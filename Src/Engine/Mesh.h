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
    virtual ~CMesh() = default;

    _DEFINE_GETTER_SETTER(Name, std::string)

    virtual CGeneralMeshDataTest getMeshData() const = 0; // FIXME: use pointer or not? which mean will the data be updated after returned?

private:
	std::string m_Name = "Default Mesh";
};

class CMeshTriangleList : public CMesh
{
public:
    _DEFINE_PTR(CMeshTriangleList);

    virtual CGeneralMeshDataTest getMeshData() const override;

    void addTriangles(std::vector<glm::vec3> vPosSet, std::vector<glm::vec3> vNormalSet);

private:
    CGeneralMeshDataTest m_Data;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicCube : public CMesh
{
public:
    _DEFINE_PTR(CMeshBasicCube);

    static CGeneralMeshDataTest MeshData;

    virtual CGeneralMeshDataTest getMeshData() const override;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicSphere : public CMesh
{
public:
    _DEFINE_PTR(CMeshBasicCube);

    static CGeneralMeshDataTest MeshData;

    virtual CGeneralMeshDataTest getMeshData() const override;
};