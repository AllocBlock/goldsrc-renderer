#pragma once
#include <glm/glm.hpp>

struct SAABB
{
    SAABB() = default;
    SAABB(const glm::vec3& vMin, const glm::vec3& vMax): Min(vMin), Max(vMax) {}

    static SAABB createByCenterExtent(const glm::vec3& vCenter, const glm::vec3& vExtent)
    {
        glm::vec3 HalfExtent = vExtent * 0.5f;
        return SAABB(vCenter - HalfExtent, vCenter + HalfExtent);
    }

    bool IsValid = true;
    glm::vec3 Min = glm::vec3(0.0f);
    glm::vec3 Max = glm::vec3(0.0f);

    static const SAABB InvalidAABB;

    void applyUnion(const SAABB& vOther)
    {
        Min.x = glm::min<float>(Min.x, vOther.Min.x);
        Min.y = glm::min<float>(Min.y, vOther.Min.y);
        Min.z = glm::min<float>(Min.z, vOther.Min.z);
        Max.x = glm::max<float>(Max.x, vOther.Max.x);
        Max.y = glm::max<float>(Max.y, vOther.Max.y);
        Max.z = glm::max<float>(Max.z, vOther.Max.z);
    }
};
