#include "RenderPassGraphUI.h"
#include "Random.h"

#include <imgui.h>

// based on https://gist.github.com/ocornut/7e9b3ec566a333d725d4

std::vector<std::pair<std::string, std::function<vk::IRenderPass::Ptr()>>> gRegisteredPassSet = {};

void CRenderPassGraphUI::registerRenderPass(const std::string& vName, std::function<vk::IRenderPass::Ptr()> vCreateFunction)
{
    gRegisteredPassSet.push_back({ vName, vCreateFunction });
}

glm::vec2 __toGlm(ImVec2 v) { return glm::vec2(v.x, v.y); }
ImVec2 __toImgui(glm::vec2 v) { return ImVec2(v.x, v.y); }

void CRenderPassGraphUI::__drawGrid()
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

void CRenderPassGraphUI::__drawLink(const SRenderPassGraphLink& vLink)
{
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    
    glm::vec2 p1 = m_NodePortPosMap.at(vLink.Source.NodeId).Output.at(vLink.Source.Name);
    glm::vec2 p2 = m_NodePortPosMap.at(vLink.Destination.NodeId).Input.at(vLink.Destination.Name);
    glm::vec2 p11 = p1 + glm::vec2(50, 0);
    glm::vec2 p22 = p2 + glm::vec2(-50, 0);

    pDrawList->AddBezierCurve(__toImgui(p1), __toImgui(p11), __toImgui(p22), __toImgui(p2), IM_COL32(200, 200, 100, 255), 3.0f);
}

void CRenderPassGraphUI::__drawNode(size_t vId, SRenderPassGraphNode& vioNode, glm::vec2 vCanvasOffset)
{
    const float LineHeight = ImGui::GetTextLineHeight();
    const float NODE_SLOT_RADIUS = 4.0f;
    const float NODE_WINDOW_PADDING = 8.0f;
    const float HeaderHeight = LineHeight + 8.0f;
    const auto HeaderBackgroundColor = IM_COL32(80, 0, 0, 255);
    const float Rounding = 4.0f;

    const auto PortTextColor = IM_COL32(255, 255, 255, 255);
    const float WidgetWidth = 120.0f; // dynamic calculating ti avoid overlap or too empty

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    ImGui::PushID(vId);
    const glm::vec2 NodeCanvasPos = vCanvasOffset + vioNode.Pos;

    float ContentPadding = 8.0f;
    float ContentHeight = LineHeight * glm::max(vioNode.InputSet.size(), vioNode.OutputSet.size()) + ContentPadding * 2;

    // work on background
    pDrawList->ChannelsSetCurrent(1);
    
    // draw whole node background and boarder
    ImU32 node_bg_color = (m_HoveredNode == vId || (m_HoveredNode == -1 && m_SelectedNodeID == vId)) ? IM_COL32(75, 75, 75, 255) : IM_COL32(60, 60, 60, 255);
    glm::vec2 NodeCanvasSize = glm::vec2(WidgetWidth, HeaderHeight + ContentHeight);
    glm::vec2 NodeCanvasEnd = NodeCanvasPos + NodeCanvasSize;
    pDrawList->AddRectFilled(__toImgui(NodeCanvasPos), __toImgui(NodeCanvasEnd), node_bg_color, Rounding);
    pDrawList->AddRect(__toImgui(NodeCanvasPos), __toImgui(NodeCanvasEnd), IM_COL32(100, 100, 100, 255), Rounding);

    // draw header background
    pDrawList->AddRectFilled(__toImgui(NodeCanvasPos), __toImgui(NodeCanvasPos + glm::vec2(WidgetWidth, HeaderHeight)), HeaderBackgroundColor, Rounding, ImDrawFlags_RoundCornersTop);

    // draw header texts
    pDrawList->ChannelsSetCurrent(2); // Foreground
    ImGuiStyle& style = ImGui::GetStyle();
    float TextHeight = ImGui::CalcTextSize(vioNode.Name.c_str()).y + style.FramePadding.y * 2.0f;
    ImGui::SetCursorScreenPos(__toImgui(NodeCanvasPos + glm::vec2(NODE_WINDOW_PADDING, (HeaderHeight - TextHeight) * 0.5f)));
    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::BeginGroup(); // Lock horizontal position
    ImGui::Text(vioNode.Name.c_str());
    ImGui::EndGroup();

    // draw ports
    m_NodePortPosMap[vId] = SPortPos();
    glm::vec2 CurInputPortPos = NodeCanvasPos + glm::vec2(0.0f, HeaderHeight + ContentPadding + LineHeight * 0.5);
    glm::vec2 CurOutputPortPos = CurInputPortPos + glm::vec2(WidgetWidth, 0.0f);

    for (const auto& InputName : vioNode.InputSet)
    {
        m_NodePortPosMap[vId].Input[InputName] = CurInputPortPos;
        pDrawList->AddCircleFilled(__toImgui(CurInputPortPos), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));

        ImVec2 TextSize = ImGui::CalcTextSize(InputName.c_str());
        pDrawList->AddText(__toImgui(CurInputPortPos + glm::vec2(NODE_SLOT_RADIUS + 4.0f, -TextSize.y * 0.5)), PortTextColor, InputName.c_str());

        CurInputPortPos.y += LineHeight;
    }

    for (const auto& OutputName : vioNode.OutputSet)
    {
        m_NodePortPosMap[vId].Output[OutputName] = CurOutputPortPos;
        pDrawList->AddCircleFilled(__toImgui(CurOutputPortPos), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));

        ImVec2 TextSize = ImGui::CalcTextSize(OutputName.c_str());
        pDrawList->AddText(__toImgui(CurOutputPortPos + glm::vec2(-NODE_SLOT_RADIUS - 4.0f - TextSize.x, -TextSize.y * 0.5)), PortTextColor, OutputName.c_str());

        CurOutputPortPos.y += LineHeight;
    }

    // Save the size of what we have emitted and whether any of the widgets are being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    vioNode.Size = __toGlm(ImGui::GetItemRectSize()) + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
    glm::vec2 node_rect_max = NodeCanvasPos + vioNode.Size;

    // draw invisable button for interaction
    pDrawList->ChannelsSetCurrent(0); // Background
    ImGui::SetCursorScreenPos(__toImgui(NodeCanvasPos));
    
    ImGui::InvisibleButton("node", __toImgui(NodeCanvasSize));
    if (ImGui::IsItemHovered())
    {
        m_HoveredNode = vId;
    }
    bool node_moving_active = ImGui::IsItemActive();
    if (node_widgets_active || node_moving_active)
        m_SelectedNodeID = vId;
    if (node_moving_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        vioNode.Pos = vioNode.Pos + __toGlm(io.MouseDelta);

    ImGui::PopID();
}

bool CRenderPassGraphUI::hasPass(size_t vNodeId)
{
    return m_pGraph->NodeMap.find(vNodeId) != m_pGraph->NodeMap.end();
}

size_t CRenderPassGraphUI::addNode(const std::string& vName, const std::vector<std::string>& vInputSet,
    const std::vector<std::string>& vOutputSet)
{
    const float Margin = 20.0f;

    glm::vec2 Pos = glm::vec2(0.0f);
    if (m_AABB.has_value())
    {
        Pos = glm::vec2(m_AABB.value().Max.x + Margin, Random::GenerateFloat() * 300.0f);
    }

    SRenderPassGraphNode Node;
    Node.Name = vName;
    Node.Pos = Pos;
    Node.InputSet = vInputSet;
    Node.OutputSet = vOutputSet;

    if (!m_AABB.has_value())
        m_AABB = Node.getAABB();
    else
        m_AABB.value().applyUnion(Node.getAABB());

    m_pGraph->NodeMap[m_CurNodeIndex] = std::move(Node);
    return m_CurNodeIndex++;
}

void CRenderPassGraphUI::addLink(size_t vStartNodeId, const std::string& vStartPortName, size_t vEndNodeId,
    const std::string& vEndPortName)
{
    _ASSERTE(hasPass(vStartNodeId));
    _ASSERTE(hasPass(vEndNodeId));
    _ASSERTE(m_pGraph->NodeMap.at(vStartNodeId).hasOutput(vStartPortName));
    _ASSERTE(m_pGraph->NodeMap.at(vEndNodeId).hasInput(vEndPortName));
    m_pGraph->LinkSet.push_back({ {vStartNodeId, vStartPortName}, {vEndNodeId, vEndPortName} });
}

void CRenderPassGraphUI::clear()
{
    m_pGraph->NodeMap.clear();
    m_pGraph->LinkSet.clear();
    m_pGraph->EntryPortOpt = std::nullopt;
    m_Scrolling = glm::vec2(0.0f);
}

void CRenderPassGraphUI::update()
{
    if (!m_EnableForce) return;

    const float m_WorldScale = 1.0f / 300.0f; // dpi
        
    // force graph
    float Step = 0.01f;
    std::map<size_t, glm::vec2> ForceMap;
        
    // 1. node repulsion
    for (auto pIter1 = m_pGraph->NodeMap.begin(); pIter1 != m_pGraph->NodeMap.end(); ++pIter1)
    {
        size_t Id1 = pIter1->first;
        const SRenderPassGraphNode& Node1 = pIter1->second;
        SAABB2D NodeAABB1 = Node1.getAABB();
        for (auto pIter2 = std::next(pIter1); pIter2 != m_pGraph->NodeMap.end(); ++pIter2)
        {
            size_t Id2 = pIter2->first;
            const SRenderPassGraphNode& Node2 = pIter2->second;
            SAABB2D NodeAABB2 = Node2.getAABB();

            glm::vec2 v = (NodeAABB1.getCenter() - NodeAABB2.getCenter()) * m_WorldScale;
            float d = glm::length(v);
            glm::vec2 ForceOn1Direction = d < 1e-3 ? glm::vec2(1, 0) : glm::normalize(v);

            float ForceOn1 = 500.0f / (d * d);
            ForceMap[Id1] += ForceOn1 * ForceOn1Direction;
            ForceMap[Id2] -= ForceOn1 * ForceOn1Direction;
        }
    }

    // 2. link attraction
    for (const SRenderPassGraphLink& Link : m_pGraph->LinkSet)
    {
        glm::vec2 v = (m_pGraph->NodeMap.at(Link.Destination.NodeId).getAABB().getCenter() - m_pGraph->NodeMap.at(Link.Source.NodeId).getAABB().getCenter()) * m_WorldScale;
        float d = glm::length(v);
        glm::vec2 ForceOn1Direction = d > 1e-3 ? glm::normalize(v) : glm::vec2(1, 0);

        float ForceOn1 = 10000.0f * d;
        glm::vec2 F = ForceOn1Direction * ForceOn1;
        ForceMap[Link.Source.NodeId] += F;
        ForceMap[Link.Destination.NodeId] -= F;
    }
        
    for (const auto& Pair : ForceMap)
    {
        size_t Id = Pair.first;
        if (Id == m_SelectedNodeID) continue; // dragging
        glm::vec2 F = Pair.second;
        SRenderPassGraphNode& Node = m_pGraph->NodeMap.at(Id);
            
        glm::vec2 A = F / 1.0f;
        glm::vec2 dx = A * Step * Step;

        if (glm::length(dx) < 1e-2f) continue; // remove too small force to avoid flickering
        Node.Pos = Node.Pos + dx;
    }
}

void CRenderPassGraphUI::createFromRenderPassGraph(std::vector<vk::IRenderPass::Ptr> vPassSet,
    std::vector<std::tuple<int, std::string, int, std::string>> vLinks, std::pair<int, std::string> vEntry)
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

    m_pGraph->EntryPortOpt = SRenderPassGraphPortInfo{ PassIds[vEntry.first], vEntry.second };
    addLink(SwapchainNodeId, "Main", m_pGraph->EntryPortOpt.value().NodeId, m_pGraph->EntryPortOpt.value().Name);
        
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

void CRenderPassGraphUI::_renderUIV()
{
    ImGuiIO& io = ImGui::GetIO();

    const int SidebarWidth = 200;

    ImGui::BeginChild("Visualize", ImVec2(-SidebarWidth, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::BeginGroup();
    // Create our child canvas
    ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", m_Scrolling.x, m_Scrolling.y);
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    ImGui::Checkbox("Show grid", &m_ShowGrid);
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Checkbox("Enable Force", &m_EnableForce);

    ImGui::BeginDisabled(!m_Editor.canUndo());
    if (ImGui::Button(u8"撤销"))
        m_Editor.undo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_Editor.canRedo());
    if (ImGui::Button(u8"重做"))
        m_Editor.redo();
    ImGui::EndDisabled();

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
    
    pDrawList->ChannelsSplit(3);

    // draw nodes
    for (auto& Pair : m_pGraph->NodeMap)
    {
        size_t Id = Pair.first;
        SRenderPassGraphNode& Node = Pair.second;
        __drawNode(Id, Node, CanvasOffset);
    }

    // draw links
    pDrawList->ChannelsSetCurrent(0); // Background
    for (const SRenderPassGraphLink& Link : m_pGraph->LinkSet)
    {
        __drawLink(Link); // depend on renderer node, so draw after node
    }
    pDrawList->ChannelsMerge();

    // context menu

    if (ImGui::IsAnyMouseDown() && !ImGui::IsAnyItemHovered())
        m_IsContextMenuOpen = false; // close on click
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsAnyItemHovered())
        m_IsContextMenuOpen = true;

    if (m_IsContextMenuOpen && m_HoveredNode != -1)
    {
        ImGui::OpenPopup("context_menu");
        m_SelectedNodeID = m_HoveredNode;
    }

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("context_menu"))
    {
        glm::vec2 scene_pos = __toGlm(ImGui::GetMousePosOnOpeningCurrentPopup()) - CanvasOffset;
        if (m_SelectedNodeID != -1)
        {
            SRenderPassGraphNode& Node = m_pGraph->NodeMap.at(m_SelectedNodeID);
            ImGui::Text("Node '%s'", Node.Name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
            if (ImGui::MenuItem("Delete", NULL, false, true))
            {
                m_Editor.deleteNode(m_SelectedNodeID);
                m_IsContextMenuOpen = false;
                m_SelectedNodeID = -1;
                m_HoveredNode = -1;
            }
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


    // Draw sidebar
    ImGui::BeginChild("sidebar", ImVec2(SidebarWidth, 0));
    if (ImGui::BeginTabBar("sidebartab"))
    {
        if (ImGui::BeginTabItem(u8"当前"))
        {
            for (const auto& Pair : m_pGraph->NodeMap)
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
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(u8"全部"))
        {
            if (gRegisteredPassSet.empty())
                ImGui::Text(u8"未注册任何渲染Pass...");
            else
            {
                for (const auto& Pair : gRegisteredPassSet)
                {
                    ImGui::Text(Pair.first.c_str());
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    // TODO: update aabb
}
