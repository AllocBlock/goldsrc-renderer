#pragma once
#include <string>
#include <vector>

#include "RenderPassGraph.h"
#include "RenderPass.h"

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

    void applyUnion(const SAABB2D& vOther)
    {
        Min.x = glm::min<float>(Min.x, vOther.Min.x);
        Min.y = glm::min<float>(Min.y, vOther.Min.y);
        Max.x = glm::max<float>(Max.x, vOther.Max.x);
        Max.y = glm::max<float>(Max.y, vOther.Max.y);
    }
};

struct SRenderPassGraphNode
{
    std::string Name;
    glm::vec2 Pos, Size; // Size is auto-updating, so no need to set it
    std::vector<std::string> InputSet, OutputSet;

    SAABB2D getAABB() const
    {
        return SAABB2D(Pos, Pos + Size);
    }

    bool hasInput(const std::string& vName) const
    {
        for (const std::string& Name : InputSet)
            if (Name == vName) return true;
    }

    bool hasOutput(const std::string& vName) const
    {
        for (const std::string& Name : OutputSet)
            if (Name == vName) return true;
    }

    size_t getInputIndex(const std::string& vName) const
    {
        for (size_t i = 0; i < InputSet.size(); ++i)
            if (InputSet[i] == vName)
                return i;
        throw std::runtime_error("Port not found");
    }

    size_t getOutputIndex(const std::string& vName) const
    {
        for (size_t i = 0; i < OutputSet.size(); ++i)
            if (OutputSet[i] == vName)
                return i;
        throw std::runtime_error("Port not found");
    }

    glm::vec2 getInputSlotPos(const std::string& vName) const
    {
        size_t Index = getInputIndex(vName);
        float OffsetY = (Size.y / float(InputSet.size())) * (Index + 0.5f);
        return Pos + glm::vec2(0, OffsetY);
    }

    glm::vec2 getOutputSlotPos(const std::string& vName) const
    {
        size_t Index = getOutputIndex(vName);
        float OffsetY = (Size.y / float(OutputSet.size())) * (Index + 0.5f);
        return Pos + glm::vec2(Size.x, OffsetY);
    }
};

struct SRenderPassGraphPortInfo
{
    size_t NodeId;
    std::string Name;
};

struct SRenderPassGraphLink
{
    SRenderPassGraphPortInfo Source;
    SRenderPassGraphPortInfo Destination;
};

class CRenderPassGraph : public IDrawableUI
{
public:
    bool hasPass(size_t vNodeId)
    {
        return m_NodeMap.find(vNodeId) != m_NodeMap.end();
    }

    size_t addNode(const std::string& vName, const std::vector<std::string>& vInputSet, const std::vector<std::string>& vOutputSet)
    {
        const float Margin = 20.0f;

        glm::vec2 Pos = glm::vec2(0.0f);
        if (m_AABB.has_value())
        {
            glm::vec2 Pos = glm::vec2(m_AABB.value().Max.x + Margin, 0.0f);
        }

        SRenderPassGraphNode Node;
        Node.Name = vName;
        Node.Pos = Pos;
        Node.InputSet = vInputSet;
        Node.OutputSet = vOutputSet;

        if (!m_AABB.has_value())
            m_AABB = Node.getAABB();
        else
            m_AABB->applyUnion(Node.getAABB());

        m_NodeMap[m_CurNodeIndex] = std::move(Node);
        return m_CurNodeIndex++;
    }

    void addLink(size_t vStartNodeId, const std::string& vStartPortName, size_t vEndNodeId, const std::string& vEndPortName)
    {
        _ASSERTE(hasPass(vStartNodeId));
        _ASSERTE(hasPass(vEndNodeId));
        _ASSERTE(m_NodeMap.at(vStartNodeId).hasOutput(vStartPortName));
        _ASSERTE(m_NodeMap.at(vEndNodeId).hasInput(vEndPortName));
        m_LinkSet.push_back({ {vStartNodeId, vStartPortName}, {vEndNodeId, vEndPortName} });
    }

    void clear()
    {
        m_NodeMap.clear();
        m_LinkSet.clear();
        m_EntryPortOpt = std::nullopt;
        m_Scrolling = glm::vec2(0.0f);
    }

    // FIXME: temp, when link of pass is done in graph, this ugly function can be removed
    void createFromRenderPassGraph(std::vector<vk::IRenderPass::Ptr> vPassSet, std::vector<std::tuple<int, std::string, int, std::string>> vLinks, std::pair<int, std::string> vEntry)
    {
        // nodes
        size_t SwapchainNodeId = addNode("Swapchain Source", {}, { "Main" }); // swap chain
        
        std::vector<size_t> PassIds(vPassSet.size()); // index to id
        for (size_t i = 0; i < vPassSet.size(); ++i)
        {
            std::string PassName = vPassSet[i]->getNameV();

            auto pPortSet = vPassSet[i]->getPortSet();
            std::vector<std::string> InputPortSet, OutputPortSet;
            for (size_t i = 0; i < pPortSet->getInputPortNum(); ++i)
                InputPortSet.emplace_back(pPortSet->getInputPort(i)->getName());
            for (size_t i = 0; i < pPortSet->getOutputPortNum(); ++i)
                OutputPortSet.emplace_back(pPortSet->getOutputPort(i)->getName());

            PassIds[i] = addNode(PassName, InputPortSet, OutputPortSet);
        }

        // links

        const size_t InvalidIndex = std::numeric_limits<size_t>::max();

        m_EntryPortOpt = SRenderPassGraphPortInfo{ PassIds[vEntry.first], vEntry.second };
        addLink(SwapchainNodeId, "Main", m_EntryPortOpt.value().NodeId, m_EntryPortOpt.value().Name);
        
        for (const auto& Link : vLinks)
        {
            int StartPassIndex = std::get<0>(Link);
            std::string StartPortName = std::get<1>(Link);
            int EndPassIndex = std::get<2>(Link);
            std::string EndPortName = std::get<3>(Link);

            size_t StartNodeId = PassIds[StartPassIndex];
            size_t EndNodeId = PassIds[EndPassIndex];
            addLink(StartNodeId, StartPortName, EndNodeId, EndPortName);
        }
    }

    virtual void _renderUIV() override;
private:
    void __drawGrid();
    void __drawLink(const SRenderPassGraphLink& vLink, glm::vec2 vCanvasOffset);
    void __drawNode(size_t vId, SRenderPassGraphNode& vioNode, glm::vec2 vCanvasOffset);

    size_t m_CurNodeIndex = 0;
    std::map<size_t, SRenderPassGraphNode> m_NodeMap;
    std::vector<SRenderPassGraphLink> m_LinkSet;
    std::optional<SRenderPassGraphPortInfo> m_EntryPortOpt = std::nullopt;
    
    std::optional<SAABB2D> m_AABB = std::nullopt;
    glm::vec2 m_Scrolling = glm::vec2(0.0f);
    int m_SelectedNodeID = -1;
    bool m_ShowGrid = true;
    bool m_IsContextMenuOpen = false;

    int m_HoveredNode = -1;
};