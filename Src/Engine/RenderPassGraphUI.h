#pragma once
#include "RenderPassGraph.h"
#include "RenderPass.h"
#include "RenderPassGraphEditor.h"
#include "Timer.h"

#include <string>
#include <vector>
#include <deque>
#include <glm/glm.hpp>

class CRenderPassGraphUI : public IDrawableUI
{
public:
    static void registerRenderPass(const std::string& vName, std::function<vk::IRenderPass::Ptr()> vCreateFunction);

    virtual void _renderUIV() override;

    void setGraph(ptr<SRenderPassGraph> vGraph) { m_pGraph = vGraph; m_Editor.setGraph(vGraph); }
    void update();

private:
    void __drawGrid();
    void __drawLink(size_t vLinkIndex, const SRenderPassGraphLink& vLink);
    void __drawNode(size_t vId, SRenderPassGraphNode& vioNode, glm::vec2 vCanvasOffset);

    // TODO: how to manage these copied function?
    bool __isNodeSelected(size_t vNodeId) const;
    bool __isLinkSelected(size_t vLinkIndex) const;
    void __setSelectedNode(size_t vNodeId);
    void __setSelectedLink(size_t vLinkIndex);
    bool __isNodeHovered(size_t vNodeId) const;
    bool __isLinkHovered(size_t vLinkIndex) const;
    void __setHoveredNode(size_t vNodeId);
    void __setHoveredLink(size_t vLinkIndex);

    ptr<SRenderPassGraph> m_pGraph = nullptr;

    glm::vec2 m_Scrolling = glm::vec2(0.0f);
    bool m_ShowGrid = true;
    bool m_IsContextMenuOpen = false;

    enum class EItemType
    {
        NODE,
        LINK
    };

    struct SItemRef
    {
        size_t Id;
        EItemType Type;
    };

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

    CRenderPassGraphEditor m_Editor;
};