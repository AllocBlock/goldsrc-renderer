#include "Transform.h"

#include <glm/ext/matrix_transform.hpp>

glm::mat4 STransform::getModelMat() const
{
    glm::mat4 Result = glm::mat4(1.0f);
    Result = glm::translate(Result, Translate); // be care that the api applys reversely, translate is the last
    Result = glm::rotate(Result, glm::radians(Rotate.x), glm::vec3(1.0, 0.0, 0.0));
    Result = glm::rotate(Result, glm::radians(Rotate.y), glm::vec3(0.0, 1.0, 0.0));
    Result = glm::rotate(Result, glm::radians(Rotate.z), glm::vec3(0.0, 0.0, 1.0));
    Result = glm::scale(Result, Scale);
    return Result;
}
