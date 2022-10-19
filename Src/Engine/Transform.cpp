#include "Transform.h"

#include <glm/ext/matrix_transform.hpp>

glm::vec3 STransform::getAbsoluteTranslate() const
{
    if (pParent)
        return pParent->getAbsoluteTranslate() + Translate;
    else
        return Translate;
}

CRotator STransform::getAbsoluteRotate() const
{
    if (pParent)
        return pParent->getAbsoluteRotate() * Rotate;
    else
        return Rotate;
}

glm::vec3 STransform::getAbsoluteScale() const
{
    if (pParent)
        return pParent->getAbsoluteScale() + Scale;
    else
        return Scale;
}

glm::mat4 STransform::getRelativeModelMat4() const
{
    glm::mat4 Result = glm::mat4(1.0f);
    Result = glm::translate(Result, Translate); // be care that the api applys reversely, translate is the last
    Result *= Rotate.getRotateMatrix();
    Result = glm::scale(Result, Scale);
    return Result;
}

glm::mat4 STransform::getAbsoluteModelMat4() const
{
    if (pParent)
        return pParent->getAbsoluteModelMat4() * getRelativeModelMat4();
    else
        return getRelativeModelMat4();
}

glm::vec3 STransform::applyRelativeOnPoint(glm::vec3 vPoint) const
{
    return glm::vec3(__applyRelative(glm::vec4(vPoint, 1.0f)));
}

glm::vec3 STransform::applyRelativeOnVector(glm::vec3 vVector) const
{
    return glm::vec3(__applyRelative(glm::vec4(vVector, 0.0f)));
}

glm::vec4 STransform::__applyRelative(glm::vec4 vValue) const
{
    return getRelativeModelMat4() * vValue;
}

glm::vec3 STransform::applyAbsoluteOnPoint(glm::vec3 vPoint) const
{
    return glm::vec3(__applyAbsolute(glm::vec4(vPoint, 1.0f)));
}

glm::vec3 STransform::applyAbsoluteOnVector(glm::vec3 vVector) const
{
    return glm::vec3(__applyAbsolute(glm::vec4(vVector, 0.0f)));
}

glm::vec4 STransform::__applyAbsolute(glm::vec4 vValue) const
{
    return getAbsoluteModelMat4() * vValue;
}