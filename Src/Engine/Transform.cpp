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

glm::mat4 STransform::getRelativeModelMat() const
{
    glm::mat4 Result = glm::mat4(1.0f);
    Result = glm::translate(Result, Translate); // be care that the api applys reversely, translate is the last
    Result *= Rotate.getRotateMatrix();
    Result = glm::scale(Result, Scale);
    return Result;
}

glm::mat4 STransform::getAbsoluteModelMat() const
{
    if (pParent)
        return pParent->getAbsoluteModelMat() * getRelativeModelMat();
    else
        return getRelativeModelMat();
}