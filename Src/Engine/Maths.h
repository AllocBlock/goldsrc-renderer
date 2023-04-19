#pragma once
#include <array>
#include <map>
#include <set>
#include <vector>
#include <glm/glm.hpp>
#include "BoundingBox.h"

namespace Math
{
    const float Acc = 1e-5f;
    
    struct SAABB2D
    {
        SAABB2D() = default;
        SAABB2D(const glm::vec2& vMin, const glm::vec2& vMax) : Min(vMin), Max(vMax) {}

        static SAABB2D createByCenterExtent(const glm::vec2& vCenter, const glm::vec2& vExtent)
        {
            glm::vec2 HalfExtent = vExtent * 0.5f;
            return SAABB2D(vCenter - HalfExtent, vCenter + HalfExtent);
        }

        glm::vec2 Min = glm::vec2(0.0f);
        glm::vec2 Max = glm::vec2(0.0f);

        glm::vec2 getCenter() const
        {
            return (Min + Max) * 0.5f;
        }

        void applyUnion(const SAABB2D& vOther)
        {
            Min.x = glm::min<float>(Min.x, vOther.Min.x);
            Min.y = glm::min<float>(Min.y, vOther.Min.y);
            Max.x = glm::max<float>(Max.x, vOther.Max.x);
            Max.y = glm::max<float>(Max.y, vOther.Max.y);
        }

        float distanceTo(const SAABB2D& vOther) const
        {
            float dx = __calcSegmentDistance(Min.x, Max.x, vOther.Min.x, vOther.Max.x);
            float dy = __calcSegmentDistance(Min.y, Max.y, vOther.Min.y, vOther.Max.y);
            return glm::sqrt(dx * dx + dy * dy);
        }

        static bool intersection(const SAABB2D& v1, const SAABB2D& v2, SAABB2D& voAABB)
        {
            float MinX = glm::max<float>(v1.Min.x, v2.Min.x);
            float MinY = glm::max<float>(v1.Min.y, v2.Min.y);
            float MaxX = glm::min<float>(v1.Max.x, v2.Max.x);
            float MaxY = glm::min<float>(v1.Max.y, v2.Max.y);
            if (MinX >= MaxX) return false;
            if (MinY >= MaxY) return false;
            voAABB = SAABB2D(glm::vec2(MinX, MinY), glm::vec2(MaxX, MaxY));
            return true;
        }
    private:
        float __calcSegmentDistance(float a1, float a2, float b1, float b2) const
        {
            if (a1 > b2) return a1 - b2;
            if (b1 > a2) return b1 - a2;
            return 0.0f;
        }
    };

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

    class CDirectedGraph
    {
    public:
        void addNode(size_t vNode) 
        {
            m_NodeSet.insert(vNode);
        }

        void addEdge(size_t vStartNode, size_t vEndNode)
        {
            _ASSERTE(hasNode(vStartNode));
            _ASSERTE(hasNode(vEndNode));
            m_EdgeSet.insert({ vStartNode, vEndNode });
        }

        bool hasNode(size_t vNode) const
        {
            return m_NodeSet.find(vNode) != m_NodeSet.end();
        }

        bool hasLoop() const
        {
            // init
            std::map<size_t, size_t> NodeInDegreeMap;
            std::map<size_t, std::vector<size_t>> NodeDependencyMap;
            for (size_t Node : m_NodeSet)
            {
                NodeInDegreeMap[Node] = 0;
                NodeDependencyMap[Node] = {};
            }
            for (const SEdge& Edge : m_EdgeSet)
            {
                NodeInDegreeMap[Edge.EndNode]++;
                NodeDependencyMap[Edge.StartNode].push_back(Edge.EndNode);
            }

            // topological sort
            while(!NodeInDegreeMap.empty())
            {
                bool FoundNode = false;
                for (const auto& Pair : NodeInDegreeMap)
                {
                    size_t Node = Pair.first;
                    size_t InDegree = Pair.second;
                    if (InDegree == 0)
                    {
                        for (size_t DependNode : NodeDependencyMap[Node])
                        {
                            NodeInDegreeMap[DependNode]--;
                        }
                        NodeInDegreeMap.erase(Node);
                        FoundNode = true;
                        break;
                    }
                }
                if (!FoundNode) // no more zero-degree node means there is loop
                    return true;
            }
            return false;
        }

    private:
        struct SEdge
        {
            size_t StartNode;
            size_t EndNode;

            bool operator < (const SEdge& vOther) const
            {
                if (StartNode == vOther.StartNode) return EndNode < vOther.EndNode;
                else return StartNode < vOther.StartNode;
            }
        };

        std::set<size_t> m_NodeSet;
        std::set<SEdge> m_EdgeSet;
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
