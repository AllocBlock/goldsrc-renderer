#pragma once
#include "RenderPassGraph.h"
#include "RenderPass.h"
#include "RenderPassGraphEditor.h"

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
    bool hasPass(size_t vNodeId);
    void clear();
    void update();

    // FIXME: these function should not be here
    size_t addNode(const std::string& vName, const std::vector<std::string>& vInputSet, const std::vector<std::string>& vOutputSet);
    void addLink(size_t vStartNodeId, const std::string& vStartPortName, size_t vEndNodeId, const std::string& vEndPortName);
    void createFromRenderPassGraph(std::vector<vk::IRenderPass::Ptr> vPassSet,
        std::vector<std::tuple<int, std::string, int, std::string>> vLinks, std::pair<int, std::string> vEntry);

private:
    void __drawGrid();
    void __drawLink(const SRenderPassGraphLink& vLink);
    void __drawNode(size_t vId, SRenderPassGraphNode& vioNode, glm::vec2 vCanvasOffset);

    ptr<SRenderPassGraph> m_pGraph = nullptr;
    size_t m_CurNodeIndex = 0;

    std::optional<SAABB2D> m_AABB = std::nullopt;
    glm::vec2 m_Scrolling = glm::vec2(0.0f);
    int m_SelectedNodeID = -1;
    bool m_ShowGrid = true;
    bool m_IsContextMenuOpen = false;

    int m_HoveredNode = -1;

    // temp data
    struct SPortPos
    {
        std::map<std::string, glm::vec2> Input;
        std::map<std::string, glm::vec2> Output;
    };
    std::map<size_t, SPortPos> m_NodePortPosMap;

    bool m_EnableForce = true;

    CRenderPassGraphEditor m_Editor;
};