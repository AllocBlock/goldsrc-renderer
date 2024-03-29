#pragma once
#include <map>
#include <optional>
#include <string>
#include <glm/glm.hpp>

struct SRenderPassGraphNode
{
    std::string Name;
    glm::vec2 Pos = glm::vec2(0.0f);

    SRenderPassGraphNode() = default;
    SRenderPassGraphNode(const std::string vName, glm::vec2 vPos = glm::vec2(0.0f))
    {
        _ASSERTE(!vName.empty());
        Name = vName;
        Pos = vPos;
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
    std::optional<SRenderPassGraphPortInfo> OutputPort = std::nullopt;

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
        if (!OutputPort.has_value())
        {
            voReason = u8"δָ�����";
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