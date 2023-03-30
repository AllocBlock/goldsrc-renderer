#pragma once
#include <glm/glm.hpp>
#include "BoundingBox.h"

namespace Math
{
    const float Acc = 1e-5f;

    class Plane
    {
    public:
        Plane(const glm::vec4& vParam): m_Param(vParam) {}

        static Plane createByNormalPoint(const glm::vec3& vNormal, const glm::vec3& vPoint)
        {
            float d = -glm::dot(vNormal, vPoint);
            return Plane(glm::vec4(vNormal, d));
        }

        const glm::vec4& getParam() const { return m_Param; }

    private:
        glm::vec4 m_Param;
    };
    
    bool intersectRayPlane(const glm::vec3& vOrigin, const glm::vec3& vDirection, const Plane& vPlane, float& voT);

    // counter-clockwise
    bool intersectRayTriangle(
        const glm::vec3& vOrigin, const glm::vec3& vDirection,
        const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC,
        float& voT, float& voU, float& voV);

    bool intersectRayBoundingBox(glm::vec3 vOrigin, glm::vec3 vDirection, SAABB vBB, float& voNearT, float& voFarT);
};
