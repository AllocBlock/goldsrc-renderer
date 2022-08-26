#pragma once
#include "Object.h"
#include "GeneralDataArray.h"

#include <glm/glm.hpp>
#include <vector>
#include <optional>
#include <memory>

enum class E3DObjectPrimitiveType
{
    TRIAGNLE_LIST,
    TRIAGNLE_STRIP_LIST,
    INDEXED_TRIAGNLE_LIST,
};

// AABB
struct S3DBoundingBox
{
    glm::vec3 Min;
    glm::vec3 Max;
};

class C3DObject : public CObject
{
public:
    bool getVisibleState() { return m_IsVisible; }
    ptr<IDataArray<glm::vec3>> getVertexArray() const { return m_pVertexArray; }
    ptr<IDataArray<glm::vec3>> getNormalArray() const { return m_pNormalArray; }
    ptr<IDataArray<glm::vec3>> getColorArray() const { return m_pColorArray; }
    ptr<IDataArray<glm::vec2>> getTexCoordArray() const { return m_pTexCoordArray; }
    ptr<IDataArray<uint32_t>> getTexIndexArray() const { return m_pTexIndexArray; }
    E3DObjectPrimitiveType getPrimitiveType() const { return m_PrimitiveType; }

    void setVisibleState(bool vState) { m_IsVisible = vState; }
    void setVertexArray(ptr<IDataArray<glm::vec3>> vVertexArray) { m_pVertexArray = vVertexArray; }
    void setNormalArray(ptr<IDataArray<glm::vec3>> vNormalArray) { m_pNormalArray = vNormalArray; }
    void setColorArray(ptr<IDataArray<glm::vec3>> vColorArray) { m_pColorArray = vColorArray; }
    void setTexCoordArray(ptr<IDataArray<glm::vec2>> vTexCoordArray) { m_pTexCoordArray = vTexCoordArray; }
    void setTexIndexArray(ptr<IDataArray<uint32_t>> vTexIndexArray) { m_pTexIndexArray = vTexIndexArray; }
    void setPrimitiveType(E3DObjectPrimitiveType vType) { m_PrimitiveType = vType; }

    std::optional<S3DBoundingBox> getBoundingBox() const;

protected:
    bool m_IsVisible = true;
    E3DObjectPrimitiveType m_PrimitiveType = E3DObjectPrimitiveType::TRIAGNLE_LIST;
    IDataArray<glm::vec3>::Ptr m_pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec3>::Ptr m_pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec3>::Ptr m_pColorArray = make<CGeneralDataArray<glm::vec3>>();
    IDataArray<glm::vec2>::Ptr m_pTexCoordArray = make<CGeneralDataArray<glm::vec2>>();
    IDataArray<uint32_t>::Ptr m_pTexIndexArray = make<CGeneralDataArray<glm::uint32_t>>();
};

