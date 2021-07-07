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

enum class E3DObjectEffectType
{
    NORMAL,
    SKY
};

class C3DObject : public CObject
{
public:
    std::shared_ptr<CDataArrayBase<glm::vec3>> getVertexArray() { return m_pVertexArray; }
    std::shared_ptr<CDataArrayBase<glm::vec3>> getNormalArray() { return m_pNormalArray; }
    std::shared_ptr<CDataArrayBase<glm::vec3>> getColorArray() { return m_pColorArray; }
    std::shared_ptr<CDataArrayBase<glm::vec2>> getTexCoordArray() { return m_pTexCoordArray; }
    std::shared_ptr<CDataArrayBase<uint32_t>> getTexIndexArray() { return m_pTexIndexArray; }
    E3DObjectPrimitiveType getPrimitiveType() { return m_PrimitiveType; }
    E3DObjectEffectType getEffectType() { return m_EffectType; }

    void setVertexArray(std::shared_ptr<CDataArrayBase<glm::vec3>> vVertexArray) { m_pVertexArray = vVertexArray; }
    void setNormalArray(std::shared_ptr<CDataArrayBase<glm::vec3>> vNormalArray) { m_pNormalArray = vNormalArray; }
    void setColorArray(std::shared_ptr<CDataArrayBase<glm::vec3>> vColorArray) { m_pColorArray = vColorArray; }
    void setTexCoordArray(std::shared_ptr<CDataArrayBase<glm::vec2>> vTexCoordArray) { m_pTexCoordArray = vTexCoordArray; }
    void setTexIndexArray(std::shared_ptr<CDataArrayBase<uint32_t>> vTexIndexArray) { m_pTexIndexArray = vTexIndexArray; }
    void setPrimitiveType(E3DObjectPrimitiveType vType) { m_PrimitiveType = vType; }
    void setEffectType(E3DObjectEffectType vType) { m_EffectType = vType; }

    std::optional<S3DBoundingBox> getBoundingBox() const;

protected:
    E3DObjectPrimitiveType m_PrimitiveType = E3DObjectPrimitiveType::TRIAGNLE_LIST;
    E3DObjectEffectType m_EffectType = E3DObjectEffectType::NORMAL;
    std::shared_ptr<CDataArrayBase<glm::vec3>> m_pVertexArray = std::make_shared<CGeneralDataArray<glm::vec3>>();
    std::shared_ptr<CDataArrayBase<glm::vec3>> m_pNormalArray = std::make_shared<CGeneralDataArray<glm::vec3>>();
    std::shared_ptr<CDataArrayBase<glm::vec3>> m_pColorArray = std::make_shared<CGeneralDataArray<glm::vec3>>();
    std::shared_ptr<CDataArrayBase<glm::vec2>> m_pTexCoordArray = std::make_shared<CGeneralDataArray<glm::vec2>>();
    std::shared_ptr<CDataArrayBase<uint32_t>> m_pTexIndexArray = std::make_shared<CGeneralDataArray<uint32_t>>();
};

