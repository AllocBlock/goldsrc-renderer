#pragma once
#include "Transform.h"

#include <glm/glm.hpp>

struct SAABB
{
    SAABB() = default;
    SAABB(const glm::vec3& vMin, const glm::vec3& vMax): Min(vMin), Max(vMax) {}

    bool IsValid = true;
    glm::vec3 Min = glm::vec3(0.0f);
    glm::vec3 Max = glm::vec3(0.0f);

    static const SAABB InvalidAABB;

    SAABB transform(cptr<STransform> vTransform)
    {
        glm::vec3 P1 = vTransform->applyAbsoluteOnPoint(Min);
        glm::vec3 P2 = vTransform->applyAbsoluteOnPoint(Max);

        glm::vec3 NewMin = glm::vec3(glm::min(P1.x, P2.x), glm::min(P1.y, P2.y), glm::min(P1.z, P2.z));
        glm::vec3 NewMax = glm::vec3(glm::max(P1.x, P2.x), glm::max(P1.y, P2.y), glm::max(P1.z, P2.z));
        return SAABB(NewMin , NewMax);
    }
};
