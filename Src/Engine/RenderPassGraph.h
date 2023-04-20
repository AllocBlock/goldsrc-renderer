#pragma once
#include "Maths.h"

#include <map>
#include <optional>
#include <string>
#include <glm/glm.hpp>

struct SRenderPassGraphNode
{
    std::string Name;
    glm::vec2 Pos = glm::vec2(0.0f);
    glm::vec2 Size = glm::vec2(20.0f); // Size is auto-updating, so no need to set it

    SRenderPassGraphNode() = default;
    SRenderPassGraphNode(const std::string vName, glm::vec2 vPos = glm::vec2(0.0f))
    {
        _ASSERTE(!vName.empty());
        Name = vName;
        Pos = vPos;
    }

    Math::SAABB2D getAABB() const
    {
        return Math::SAABB2D(Pos, Pos + Size);
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
    bool hasLink(size_t vLinkId) const { return LinkMap.find(vLinkId) != LinkMap.end(); }
    bool hasLink(const SRenderPassGraphLink& vLink) const
    {
        for (const auto& Pair : LinkMap)
            if (Pair.second == vLink)
                return true;
        return false;
    }

    bool isValid(std::string& voReason) const
    {
        if (!EntryPortOpt.has_value())
        {
            voReason = u8"未指定入口";
            return false;
        }
        // TODO: more validation
        return true;
    }

    bool isValid() const
    {
        std::string Temp;
        return isValid(Temp);
    }
};