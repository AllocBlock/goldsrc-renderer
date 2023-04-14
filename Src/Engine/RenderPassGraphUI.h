#pragma once
#include "RenderPassGraph.h"
#include "RenderPass.h"
#include "RenderPassGraphEditor.h"
#include "Timer.h"
#include "Maths.h"
#include "RenderPassGraphAddLinkState.h"

#include <string>
#include <glm/glm.hpp>

class CRenderPassGraphUI : public IDrawableUI
{
public:
    virtual void _renderUIV() override;

    void setGraph(ptr<SRenderPassGraph> vGraph)
    {
        m_pGraph = vGraph;
        m_Editor.setGraph(vGraph);
        m_AddLinkState.setGraph(vGraph);
    }
    void update();

private:
    enum class EItemType
    {
        NODE,
        LINK,
        PORT
    };

    struct SItemRef
    {
        EItemType Type;
        size_t Id;
        std::string Name; // only for port
        bool IsInput; // only for port
    };

    void __drawGrid();
    void __drawLink(size_t vLinkId, const SRenderPassGraphLink& vLink);
    void __drawNode(size_t vNodeId, SRenderPassGraphNode& vioNode, glm::vec2 vCanvasOffset);
    void __drawCurveAnimation(const Math::SCubicBezier2D& vCurve, unsigned vColor, float vRadius);

    // TODO: how to manage these copied function?
    bool __isItemSelected(size_t vId, EItemType vType, const std::string& vName = "", bool vIsInput = true) const;
    void __setSelectedItem(size_t vId, EItemType vType, const std::string& vName = "", bool vIsInput = true);
    bool __isItemHovered(size_t vNodeId, EItemType vType, const std::string& vName = "", bool vIsInput = true) const;
    void __setHoveredItem(size_t vId, EItemType vType, const std::string& vName = "", bool vIsInput = true);

    glm::vec2 __getPortPos(const SRenderPassGraphPortInfo& vPort, bool vIsInput)
    {
        if (vIsInput)
            return m_NodePortPosMap.at(vPort.NodeId).Input.at(vPort.Name);
        else
            return m_NodePortPosMap.at(vPort.NodeId).Output.at(vPort.Name);
    }

    ptr<SRenderPassGraph> m_pGraph = nullptr;

    glm::vec2 m_Scrolling = glm::vec2(0.0f);
    bool m_ShowGrid = true;
    bool m_IsContextMenuOpen = false;

    std::optional<SItemRef> m_HoveredItem = std::nullopt; // real-time update, override
    std::optional<SItemRef> m_DeferHoveredItem = std::nullopt; // actual hovered, defered for drawing
    std::optional<SItemRef> m_SelectedItem = std::nullopt;
    std::optional<SItemRef> m_DeferSelectedItem = std::nullopt;

    // temp data
    struct SPortPos
    {
        std::map<std::string, glm::vec2> Input;
        std::map<std::string, glm::vec2> Output;
    };
    std::map<size_t, SPortPos> m_NodePortPosMap;

    bool m_EnableForce = true;
    CTimer m_Timer;
    float m_AnimationTime = 0.0f;

    // edit
    CRenderGraphAddLinkState m_AddLinkState;
    CRenderPassGraphEditor m_Editor;
};