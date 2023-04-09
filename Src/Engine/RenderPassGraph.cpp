#include "RenderPassGraph.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
// based on https://gist.github.com/ocornut/7e9b3ec566a333d725d4

glm::vec2 __toGlm(ImVec2 v) { return glm::vec2(v.x, v.y); }
ImVec2 __toImgui(glm::vec2 v) { return ImVec2(v.x, v.y); }

void CRenderPassGraph::__drawGrid()
{
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
    float GRID_SZ = 64.0f;
    glm::vec2 CanvasPos = __toGlm(ImGui::GetCursorScreenPos());
    glm::vec2 CanvasSize = __toGlm(ImGui::GetWindowSize());
    for (float x = fmodf(m_Scrolling.x, GRID_SZ); x < CanvasSize.x; x += GRID_SZ)
    {
        pDrawList->AddLine(__toImgui(glm::vec2(x, 0.0f) + CanvasPos), __toImgui(glm::vec2(x, CanvasSize.y) + CanvasPos), GRID_COLOR);
    }
    for (float y = fmodf(m_Scrolling.y, GRID_SZ); y < CanvasSize.y; y += GRID_SZ)
    {
        pDrawList->AddLine(__toImgui(glm::vec2(0.0f, y) + CanvasPos), __toImgui(glm::vec2(CanvasSize.x, y) + CanvasPos), GRID_COLOR);
    }
}

void CRenderPassGraph::__drawLink(const SRenderPassGraphLink& vLink, glm::vec2 vCanvasOffset)
{
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    const auto& SrcNode = m_NodeMap[vLink.Source.NodeId];
    const auto& DestNode = m_NodeMap[vLink.Destination.NodeId];
    glm::vec2 p1 = vCanvasOffset + SrcNode.getOutputSlotPos(vLink.Source.Name);
    glm::vec2 p2 = vCanvasOffset + DestNode.getInputSlotPos(vLink.Destination.Name);
    glm::vec2 p11 = p1 + glm::vec2(50, 0);
    glm::vec2 p22 = p2 + glm::vec2(-50, 0);

    pDrawList->AddBezierCurve(__toImgui(p1), __toImgui(p11), __toImgui(p22), __toImgui(p2), IM_COL32(200, 200, 100, 255), 3.0f);
}

void CRenderPassGraph::__drawNode(size_t vId, SRenderPassGraphNode& vioNode, glm::vec2 vCanvasOffset)
{
    const float NODE_SLOT_RADIUS = 4.0f;
    const glm::vec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    ImGui::PushID(vId);
    glm::vec2 node_rect_min = vCanvasOffset + vioNode.Pos;

    // Display node contents first
    pDrawList->ChannelsSetCurrent(1); // Foreground
    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::SetCursorScreenPos(__toImgui(node_rect_min + NODE_WINDOW_PADDING));
    ImGui::BeginGroup(); // Lock horizontal position
    ImGui::Text("%s", vioNode.Name.c_str());
    ImGui::EndGroup();

    // Save the size of what we have emitted and whether any of the widgets are being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    vioNode.Size = __toGlm(ImGui::GetItemRectSize()) + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
    glm::vec2 node_rect_max = node_rect_min + vioNode.Size;

    // Display node box
    pDrawList->ChannelsSetCurrent(0); // Background
    ImGui::SetCursorScreenPos(__toImgui(node_rect_min));

    ImVec2 Size = __toImgui(vioNode.Size);
    if (ImGui::InvisibleButton("node", Size))
    {
        vioNode.Size = __toGlm(Size);
    }
    if (ImGui::IsItemHovered())
    {
        m_HoveredNode = vId;
        m_IsContextMenuOpen |= ImGui::IsMouseClicked(1);
    }
    bool node_moving_active = ImGui::IsItemActive();
    if (node_widgets_active || node_moving_active)
        m_SelectedNodeID = vId;
    if (node_moving_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        vioNode.Pos = vioNode.Pos + __toGlm(io.MouseDelta);

    ImU32 node_bg_color = (m_HoveredNode == vId ||  (m_HoveredNode == -1 && m_SelectedNodeID == vId)) ? IM_COL32(75, 75, 75, 255) : IM_COL32(60, 60, 60, 255);
    pDrawList->AddRectFilled(__toImgui(node_rect_min), __toImgui(node_rect_max), node_bg_color, 4.0f);
    pDrawList->AddRect(__toImgui(node_rect_min), __toImgui(node_rect_max), IM_COL32(100, 100, 100, 255), 4.0f);
    for (const auto& InputName : vioNode.InputSet)
        pDrawList->AddCircleFilled(__toImgui(vCanvasOffset + vioNode.getInputSlotPos(InputName)), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
    for (const auto& OutputName : vioNode.OutputSet)
        pDrawList->AddCircleFilled(__toImgui(vCanvasOffset + vioNode.getOutputSlotPos(OutputName)), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));

    ImGui::PopID();
}

void CRenderPassGraph::_renderUIV()
{
    ImGuiIO& io = ImGui::GetIO();

    const int ListWidth = 100;

    ImGui::BeginChild("Visualize", ImVec2(-ListWidth, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::BeginGroup();
    // Create our child canvas
    ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", m_Scrolling.x, m_Scrolling.y);
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Checkbox("Show grid", &m_ShowGrid);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
    ImGui::BeginChild("Canvas", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(); // WindowPadding
    ImGui::PushItemWidth(120.0f);

    const glm::vec2 CanvasOffset = __toGlm(ImGui::GetCursorScreenPos()) + m_Scrolling;
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    // Display grid
    if (m_ShowGrid)
        __drawGrid();

    // Display links curve
    pDrawList->ChannelsSplit(2);
    pDrawList->ChannelsSetCurrent(0); // Background
    for (const SRenderPassGraphLink& Link : m_LinkSet)
    {
        __drawLink(Link, CanvasOffset);
    }

    // Display nodes
    for (auto& Pair : m_NodeMap)
    {
        size_t Id = Pair.first;
        SRenderPassGraphNode& Node = Pair.second;
        __drawNode(Id, Node, CanvasOffset);
    }
    pDrawList->ChannelsMerge();

    // Open context menu
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || !ImGui::IsAnyItemHovered())
        {
            m_SelectedNodeID = m_HoveredNode = -1;
            m_IsContextMenuOpen = true;
        }
    if (m_IsContextMenuOpen)
    {
        ImGui::OpenPopup("context_menu");
        if (m_HoveredNode != -1)
            m_SelectedNodeID = m_HoveredNode;
    }

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("context_menu"))
    {
        glm::vec2 scene_pos = __toGlm(ImGui::GetMousePosOnOpeningCurrentPopup()) - CanvasOffset;
        if (m_SelectedNodeID != -1)
        {
            SRenderPassGraphNode& Node = m_NodeMap[m_SelectedNodeID];
            ImGui::Text("Node '%s'", Node.Name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
            if (ImGui::MenuItem("Delete", NULL, false, false)) {}
            if (ImGui::MenuItem("Copy", NULL, false, false)) {}
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        m_Scrolling = m_Scrolling + __toGlm(io.MouseDelta);

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::EndChild();


    ImGui::SameLine();


    // Draw a list of nodes on the left side
    ImGui::BeginChild("node_list", ImVec2(ListWidth, 0));
    ImGui::Text("Nodes");
    ImGui::Separator();
    for (const auto& Pair : m_NodeMap)
    {
        size_t Id = Pair.first;
        const SRenderPassGraphNode& Node = Pair.second;
        ImGui::PushID(Id);
        if (ImGui::Selectable(Node.Name.c_str(), Id == m_SelectedNodeID))
            m_SelectedNodeID = Id;
        if (ImGui::IsItemHovered())
        {
            m_HoveredNode = Id;
            m_IsContextMenuOpen |= ImGui::IsMouseClicked(1);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    // TODO: update aabb
}
