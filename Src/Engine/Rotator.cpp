#include "Rotator.h"
#include "Common.h"

CRotator CRotator::createVectorToVector(glm::vec3 vStart, glm::vec3 vTarget)
{
    vStart = glm::normalize(vStart);
    vTarget = glm::normalize(vTarget);

    float Dot = glm::dot(vStart, vTarget);
        
    if (glm::abs(Dot) > 1 - Common::Acc) return CRotator();

    glm::vec3 Cross = glm::cross(vStart, vTarget);
    glm::quat Quat = glm::quat(glm::sqrt((Dot + 1) * 0.5), Cross);
    return CRotator(Quat);
}
