#include "Transform.h"

#include <glm/ext/matrix_transform.hpp>

glm::mat4 STransform::getModelMat() const
{
    glm::mat4 Result = glm::mat4(1.0f);
    Result = glm::translate(Result, Translate); // be care that the api applys reversely, translate is the last
    Result *= Rotate.getRotateMatrix();
    Result = glm::scale(Result, Scale);
    return Result;
}
