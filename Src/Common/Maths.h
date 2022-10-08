#pragma once
#include <glm/glm.hpp>

namespace Math
{
    const float Acc = 1e-5;

    // AABB
    struct S3DBoundingBox
    {
        glm::vec3 Min;
        glm::vec3 Max;
    };

    // counter-clockwise
    bool intersectRayTriangle(
        const glm::vec3& O, const glm::vec3& D,
        const glm::vec3& A, const glm::vec3& B, const glm::vec3& C,
        float& t, float& u, float& v);

    bool intersectRayBoundingBox(glm::vec3 vOrigin, glm::vec3 vDirection, S3DBoundingBox vBB, float& voNearT, float& voFarT);
};

