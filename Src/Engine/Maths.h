#pragma once
#include <array>
#include <vector>
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

    struct SLineSegment2D
    {
        glm::vec2 Start;
        glm::vec2 End;

        float calcDistanceToPoint(const glm::vec2 vPoint) const
        {
            glm::vec2 AP = vPoint - Start;
            glm::vec2 AB = End - Start;

            float SegLength = glm::length(AB);

            float ProjectionLength = glm::dot(AP, AB) / SegLength;
            if (ProjectionLength <= 0.0f) return glm::length(AP);
            else if (ProjectionLength >= SegLength) return glm::length(End - vPoint);
            else
            {
                float BevelLength = glm::length(AP);
                return glm::sqrt(BevelLength * BevelLength - ProjectionLength * ProjectionLength);
            }
        }
    };

    struct SCubicBezier2D
    {
        std::array<glm::vec2, 4> Points;

        SCubicBezier2D(const glm::vec2& vP1, const glm::vec2& vP2, const glm::vec2& vP3, const glm::vec2& vP4)
        {
            Points = { vP1, vP2, vP3, vP4 };
        }

        glm::vec2 sample(float vT) const
        {
            _ASSERTE(vT >= 0.0f && vT <= 1.0f);
            return Points[0] * glm::pow(1.0f - vT, 3.0f) +
                   Points[1] * 3.0f * vT * glm::pow(1.0f - vT, 2.0f) +
                   Points[2] * 3.0f * (1.0f - vT) * glm::pow(vT, 2.0f) +
                   Points[3] * glm::pow(vT, 3.0f);
        }

        std::vector<SLineSegment2D> downSample(int vSegNum = 6) const
        {
            glm::vec2 LastPoint;
            std::vector<SLineSegment2D> Segs;
            for (int i = 0; i < vSegNum + 1; ++i)
            {
                float t = float(i) / float(vSegNum);
                glm::vec2 CurPoint = sample(t);
                if (i > 0)
                    Segs.push_back({ LastPoint, CurPoint });
                LastPoint = CurPoint;
            }
            return Segs;
        }

        // get approximate nearest distance of a point to cubic bezier
        float calcDistanceToPoint(const glm::vec2& vPoint, int vApproximateSegNum = 6) const
        {
            const auto& Segs = downSample(vApproximateSegNum);
            float MinD = INFINITY;
            for (const auto& Seg : Segs)
            {
                float d = Seg.calcDistanceToPoint(vPoint);
                MinD = glm::min(MinD, d);
            }
            return MinD;
        }
    };

    float smoothstepInversed(float x);

    bool intersectRayPlane(const glm::vec3& vOrigin, const glm::vec3& vDirection, const Plane& vPlane, float& voT);

    // counter-clockwise
    bool intersectRayTriangle(
        const glm::vec3& vOrigin, const glm::vec3& vDirection,
        const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC,
        float& voT, float& voU, float& voV);

    bool intersectRayAABB(glm::vec3 vOrigin, glm::vec3 vDirection, SAABB vAABB, bool vAllowInside, float& voNearT, float& voFarT);
};
