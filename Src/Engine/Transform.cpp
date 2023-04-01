#include "Transform.h"

#include <glm/ext/matrix_transform.hpp>

glm::vec3 CTransform::getAbsoluteTranslate() const
{
    if (!m_pParent.expired())
        return m_pParent.lock()->getAbsoluteTranslate() + m_Translate;
    else
        return m_Translate;
}

CRotator CTransform::getAbsoluteRotate() const
{
    if (!m_pParent.expired())
        return m_pParent.lock()->getAbsoluteRotate() * m_Rotate;
    else
        return m_Rotate;
}

glm::vec3 CTransform::getAbsoluteScale() const
{
    if (!m_pParent.expired())
        return m_pParent.lock()->getAbsoluteScale() + m_Scale;
    else
        return m_Scale;
}

glm::mat4 CTransform::getRelativeModelMat4() const
{
    glm::mat4 Result = glm::mat4(1.0f);
    Result = glm::translate(Result, m_Translate); // be care that the api applys reversely, translate is the last
    Result *= m_Rotate.getRotateMatrix();
    Result = glm::scale(Result, m_Scale);
    return Result;
}

glm::mat4 CTransform::getAbsoluteModelMat4() const
{
    if (!m_pParent.expired())
        return m_pParent.lock()->getAbsoluteModelMat4() * getRelativeModelMat4();
    else
        return getRelativeModelMat4();
}

glm::vec3 CTransform::applyRelativeOnPoint(glm::vec3 vPoint) const
{
    return glm::vec3(__applyRelative(glm::vec4(vPoint, 1.0f)));
}

glm::vec3 CTransform::applyRelativeOnVector(glm::vec3 vVector) const
{
    return glm::vec3(__applyRelative(glm::vec4(vVector, 0.0f)));
}

glm::vec4 CTransform::__applyRelative(glm::vec4 vValue) const
{
    return getRelativeModelMat4() * vValue;
}

glm::vec3 CTransform::applyAbsoluteOnPoint(glm::vec3 vPoint) const
{
    return glm::vec3(__applyAbsolute(glm::vec4(vPoint, 1.0f)));
}

glm::vec3 CTransform::applyAbsoluteOnVector(glm::vec3 vVector) const
{
    return glm::vec3(__applyAbsolute(glm::vec4(vVector, 0.0f)));
}

glm::vec4 CTransform::__applyAbsolute(glm::vec4 vValue) const
{
    return getAbsoluteModelMat4() * vValue;
}