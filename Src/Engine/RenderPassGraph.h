#pragma once
#include "RenderpassLib.h"

#include <map>
#include <optional>
#include <string>
#include <glm/glm.hpp>

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

struct SRenderPassGraphNode
{
    std::string Name;
    glm::vec2 Pos = glm::vec2(0.0f);
    glm::vec2 Size = glm::vec2(20.0f); // Size is auto-updating, so no need to set it

    SRenderPassGraphNode() = default;
    SRenderPassGraphNode(const std::string vName)
    {
        _ASSERTE(!vName.empty());
        Name = vName;
    }

    SAABB2D getAABB() const
    {
        return SAABB2D(Pos, Pos + Size);
    }
};

struct SRenderPassGraphPortInfo
{
    size_t NodeId = std::numeric_limits<size_t>::max();
    std::string Name;

    bool operator == (const SRenderPassGraphPortInfo& vOther) const
    {
        return NodeId == vOther.NodeId && Name == vOther.Name;
    }
};

struct SRenderPassGraphLink
{
    SRenderPassGraphPortInfo Source;
    SRenderPassGraphPortInfo Destination;

    bool operator == (const SRenderPassGraphLink& vOther) const
    {
        return Source == vOther.Source && Destination == vOther.Destination;
    }
};

// TIPS: Graph does not know if link is valid, as port info is defined at runtime
struct SRenderPassGraph
{
    std::map<size_t, SRenderPassGraphNode> NodeMap;
    std::map<size_t, SRenderPassGraphLink> LinkMap;
    std::optional<SRenderPassGraphPortInfo> EntryPortOpt = std::nullopt;

    bool hasNode(size_t vNodeId) const { return NodeMap.find(vNodeId) != NodeMap.end(); }
    bool hasLink(const SRenderPassGraphLink& vLink) const
    {
        for (const auto& Pair : LinkMap)
            if (Pair.second == vLink)
                return true;
        return false;
    }

    bool isValid() const
    {
        if (!EntryPortOpt.has_value())
            return false;
        // TODO: more validation
        return true;
    }
};