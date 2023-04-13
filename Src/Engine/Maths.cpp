#include "Maths.h"

//#define ENABLE_BACK_FACE_CULLING
#define ENABLE_IN_BOX_INTERSECTION

float Math::smoothstepInversed(float x)
{
    // https://www.shadertoy.com/view/MsSBRh
    return 0.5 - glm::sin(glm::asin(1.0 - 2.0 * x) / 3.0);
}

bool Math::intersectRayPlane(const glm::vec3& vOrigin, const glm::vec3& vDirection, const Plane& vPlane, float& voT)
{
    glm::vec4 Param = vPlane.getParam();
    glm::vec3 D = glm::vec3(Param);
    float d = Param.w;

    float Divide = -glm::dot(D, vOrigin) - d;
    float Divider = glm::dot(D, vDirection);
    if (glm::abs(Divider) < Math::Acc)
        return false;
    float t = Divide / Divider;
    if (t < 0.0) return false;

    voT = t;
    return true;
}

// https://gdbooks.gitbooks.io/3dcollisions/content/Chapter4/raycast_triangle.html
bool Math::intersectRayTriangle(
    const glm::vec3& vOrigin, const glm::vec3& vDirection,
    const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC,
    float& voT, float& voU, float& voV)
{
    glm::vec3 E1 = vB - vA;
    glm::vec3 E2 = vC - vA;
    glm::vec3 TriangleNormal = glm::normalize(glm::cross(E1, E2));
#ifdef ENABLE_BACK_FACE_CULLING
    // if the normal is on same direction, the triangle is backfacing
    if (glm::dot(TriangleNormal, vDirection) > 0) return false;
#endif

    Plane TrianglePlane = Plane::createByNormalPoint(TriangleNormal, vA);
    float t = 0;
    bool IsIntersected = Math::intersectRayPlane(vOrigin, vDirection, TrianglePlane, t);

    if (!IsIntersected) return false;

    // is inside triangle
    glm::vec3 Intersection = vOrigin + vDirection * t;
    glm::vec3 V = Intersection - vA;
    float u = glm::dot(V, E1) / glm::pow(glm::length(E1), 2);
    float v = glm::dot(V, E2) / glm::pow(glm::length(E2), 2);
    if (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0) return false;

    voT = t;
    voU = u;
    voV = v;
    return true;
}

// https://gdbooks.gitbooks.io/3dcollisions/content/Chapter3/raycast_aabb.html
bool Math::intersectRayAABB(glm::vec3 vOrigin, glm::vec3 vDirection, SAABB vAABB, bool vAllowInside, float& voNearT, float& voFarT)
{
    float t1 = (vAABB.Min.x - vOrigin.x) / vDirection.x;
    float t2 = (vAABB.Max.x - vOrigin.x) / vDirection.x;
    float t3 = (vAABB.Min.y - vOrigin.y) / vDirection.y;
    float t4 = (vAABB.Max.y - vOrigin.y) / vDirection.y;
    float t5 = (vAABB.Min.z - vOrigin.z) / vDirection.z;
    float t6 = (vAABB.Max.z - vOrigin.z) / vDirection.z;

    float minA = glm::min(t1, t2), minB = glm::min(t3, t4), minC = glm::min(t5, t6);
    float maxA = glm::max(t1, t2), maxB = glm::max(t3, t4), maxC = glm::max(t5, t6);

    float tmin = glm::max(glm::max(minA, minB), minC);
    float tmax = glm::min(glm::min(maxA, maxB), maxC);

    if (tmax < 0) return false; // behind
    if (tmin > tmax) return false; // not intersected

    if (tmin < 0.0 && !vAllowInside) return false; // inside
    voNearT = tmin;
    voFarT = tmax;
    return true;
}