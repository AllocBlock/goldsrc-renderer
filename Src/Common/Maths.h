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
        const glm::vec3& vOrigin, const glm::vec3& vDirection,
        const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC,
        float& voT, float& voU, float& voV);

    bool intersectRayBoundingBox(glm::vec3 vOrigin, glm::vec3 vDirection, S3DBoundingBox vBB, float& voNearT, float& voFarT);
};

