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

#define _DEFINE_MESH_DATA_ARRAY_PROPERTY(Name, Type) public: _DEFINE_GETTER_SETTER_POINTER(Name, sptr<IDataArray<##Type##>>) private: sptr<IDataArray<##Type##>> m_p##Name = make<CGeneralDataArray<Type>>();

class CMeshData
{
public:
    
    CMeshData copyDeeply();
    
    _DEFINE_GETTER_SETTER(PrimitiveType, E3DObjectPrimitiveType)

    static const uint32_t InvalidLightmapIndex;
    bool hasLightmap() const;
    SAABB getAABB() const;

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
	    virtual ~CMesh() = default;

    _DEFINE_GETTER_SETTER(Name, std::string)

    virtual CMeshData getMeshDataV() const = 0; // FIXME: use pointer or not? which mean will the data be updated after returned?
    SAABB getAABB() const;

private:
	std::string m_Name = "Default Mesh";
};

class CMeshTriangleList : public CMesh
{
public:
    
    virtual CMeshData getMeshDataV() const override;

    void addTriangles(std::vector<glm::vec3> vPosSet, std::vector<glm::vec3> vNormalSet);

    void setMeshData(CMeshData vData);

private:
    CMeshData m_Data;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicQuad : public CMesh
{
public:
    
    static CMeshData MeshData;

    virtual CMeshData getMeshDataV() const override;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicCube : public CMesh
{
public:
    
    static CMeshData MeshData;

    virtual CMeshData getMeshDataV() const override;
};

// TODO: move to basic shape, which provide mesh and collider generation
class CMeshBasicSphere : public CMesh
{
public:
    
    static CMeshData MeshData;

    virtual CMeshData getMeshDataV() const override;
};

namespace Mesh
{
    sptr<CMeshTriangleList> bakeTransform(cptr<CMesh> vMesh, cptr<CTransform> vTransform);
}
