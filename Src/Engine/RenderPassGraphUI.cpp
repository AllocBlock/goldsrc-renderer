#include "RenderPassGraphUI.h"
#include "Random.h"
#include "Maths.h"

#include <imgui.h>
#include <imgui_internal.h>

// based on https://gist.github.com/ocornut/7e9b3ec566a333d725d4

std::vector<std::pair<std::string, std::function<vk::IRenderPass::Ptr()>>> gRegisteredPassSet = {};

glm::vec2 __toGlm(ImVec2 v) { return glm::vec2(v.x, v.y); }
ImVec2 __toImgui(glm::vec2 v) { return ImVec2(v.x, v.y); }

Math::SCubicBezier2D __createLinkCurve(const glm::vec2& vStart, const glm::vec2& vEnd)
{
    glm::vec2 P2 = vStart + glm::vec2(50, 0);
    glm::vec2 P3 = vEnd + glm::vec2(-50, 0);
    return Math::SCubicBezier2D(vStart, P2, P3, vEnd);
}

void __drawLinkCurve(const Math::SCubicBezier2D& vBezier, unsigned vColor, float vThickness)
{
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    pDrawList->AddBezierCurve(__toImgui(vBezier.Points[0]), __toImgui(vBezier.Points[1]), __toImgui(vBezier.Points[2]), __toImgui(vBezier.Points[3]), vColor, vThickness);
}

void CRenderPassGraphUI::registerRenderPass(const std::string& vName, std::function<vk::IRenderPass::Ptr()> vCreateFunction)
{
    gRegisteredPassSet.push_back({ vName, vCreateFunction });
}

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

void CRenderPassGraphUI::__drawLink(size_t vLinkId, const SRenderPassGraphLink& vLink)
{
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    
    glm::vec2 Start = m_NodePortPosMap.at(vLink.Source.NodeId).Output.at(vLink.Source.Name);
    glm::vec2 End = m_NodePortPosMap.at(vLink.Destination.NodeId).Input.at(vLink.Destination.Name);
    Math::SCubicBezier2D LinkCurve = __createLinkCurve(Start, End);

    if (!m_IsContextMenuOpen)
    {
        const float HoverDistance = 6.0f;
        glm::vec2 MousePos = __toGlm(ImGui::GetMousePos());
        float d = LinkCurve.calcDistanceToPoint(MousePos);
        if (d <= HoverDistance && !m_HoveredItem.has_value()) // only when no node hovered
        {
            __setHoveredItem(vLinkId, EItemType::LINK);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                __setSelectedItem(vLinkId, EItemType::LINK);
            }
        }
    }

    auto LinkColor = IM_COL32(200, 200, 100, 255);
    float LinkThickness = 3.0f;
    bool Hovered = __isItemHovered(vLinkId, EItemType::LINK);
    if (Hovered)
    {
        LinkColor = IM_COL32(255, 255, 150, 255);
        LinkThickness = 6.0f;
    }

    __drawLinkCurve(LinkCurve, LinkColor, LinkThickness);

    // draw link animation
    unsigned BaseCircleColor = LinkColor;
    if (__isItemSelected(vLinkId, EItemType::LINK))
        __drawCurveAnimation(LinkCurve, BaseCircleColor, LinkThickness + 2.0f);

    // debug, line segment
    /*for (const auto& Seg : Bezier.downSample())
    {
        pDrawList->AddLine(__toImgui(Seg.Start), __toImgui(Seg.End), LinkColor, LinkThickness);
    }*/
}

void CRenderPassGraphUI::__drawNode(size_t vNodeId, SRenderPassGraphNode& vioNode, glm::vec2 vCanvasOffset)
{
    const float LineHeight = ImGui::GetTextLineHeight();
    const float NODE_SLOT_RADIUS = 8.0f;
    const float NODE_SLOT_HOVER_RADIUS = NODE_SLOT_RADIUS + 2.0f;
    const float NODE_WINDOW_PADDING = 8.0f;
    const float HeaderHeight = LineHeight + 8.0f;
    const auto HeaderBackgroundColor = IM_COL32(80, 0, 0, 255);
    const float Rounding = 4.0f;
    const float PortItemHeight = glm::max(ImGui::GetTextLineHeight(), NODE_SLOT_HOVER_RADIUS) + 8.0f;

    const auto PortTextColor = IM_COL32(255, 255, 255, 255);
    const float WidgetWidth = 120.0f; // dynamic calculating ti avoid overlap or too empty

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    const glm::vec2 NodeCanvasPos = vCanvasOffset + vioNode.Pos;

    float ContentPadding = 8.0f;
    float ContentHeight = PortItemHeight * glm::max(vioNode.InputSet.size(), vioNode.OutputSet.size()) + ContentPadding * 2;
    
    glm::vec2 NodeCanvasSize = glm::vec2(WidgetWidth, HeaderHeight + ContentHeight);
    glm::vec2 NodeCanvasEnd = NodeCanvasPos + NodeCanvasSize;
    
    ImGui::PushID(vNodeId);
    // draw invisable button for interaction
    pDrawList->ChannelsSetCurrent(0);
    ImGui::SetCursorScreenPos(__toImgui(NodeCanvasPos));

    ImGui::InvisibleButton("node", __toImgui(NodeCanvasSize));
    if (!m_IsContextMenuOpen && ImGui::IsItemHovered())
    {
        __setHoveredItem(vNodeId, EItemType::NODE);
    }

    // dragging
    if (!m_IsAddingLink)
    {
        bool IsMoving = ImGui::IsItemActive();
        if (IsMoving)
            __setSelectedItem(vNodeId, EItemType::NODE);
        if (IsMoving && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            vioNode.Pos = vioNode.Pos + __toGlm(io.MouseDelta);
    }

    // work on background
    pDrawList->ChannelsSetCurrent(1);
    
    // draw whole node background and boarder
    ImU32 BackgroundColor = IM_COL32(60, 60, 60, 255);
    if (__isItemSelected(vNodeId, EItemType::NODE))
        BackgroundColor = IM_COL32(100, 100, 100, 255);
    else if (__isItemHovered(vNodeId, EItemType::NODE))
        BackgroundColor = IM_COL32(75, 75, 75, 255);
    pDrawList->AddRectFilled(__toImgui(NodeCanvasPos), __toImgui(NodeCanvasEnd), BackgroundColor, Rounding);
    pDrawList->AddRect(__toImgui(NodeCanvasPos), __toImgui(NodeCanvasEnd), IM_COL32(100, 100, 100, 255), Rounding);

    // draw header background
    pDrawList->AddRectFilled(__toImgui(NodeCanvasPos), __toImgui(NodeCanvasPos + glm::vec2(WidgetWidth, HeaderHeight)), HeaderBackgroundColor, Rounding, ImDrawFlags_RoundCornersTop);

    // draw header texts
    pDrawList->ChannelsSetCurrent(2); // Foreground
    ImGuiStyle& style = ImGui::GetStyle();
    float TextHeight = ImGui::CalcTextSize(vioNode.Name.c_str()).y + style.FramePadding.y * 2.0f;
    ImGui::SetCursorScreenPos(__toImgui(NodeCanvasPos + glm::vec2(NODE_WINDOW_PADDING, (HeaderHeight - TextHeight) * 0.5f)));
    ImGui::BeginGroup(); // Lock horizontal position
    ImGui::Text(vioNode.Name.c_str());
    ImGui::EndGroup();

    // draw ports
    m_NodePortPosMap[vNodeId] = SPortPos();
    glm::vec2 CurInputPortPos = NodeCanvasPos + glm::vec2(0.0f, HeaderHeight + ContentPadding + PortItemHeight * 0.5);
    glm::vec2 CurOutputPortPos = CurInputPortPos + glm::vec2(WidgetWidth, 0.0f);

    // TODO: merge common part of input/output
    for (const auto& InputName : vioNode.InputSet)
    {
        float Radius = NODE_SLOT_RADIUS;
        unsigned Color = IM_COL32(150, 150, 150, 150);

        const float HoverRadius = NODE_SLOT_HOVER_RADIUS + 2.0f;
        const float Distance = glm::length(__toGlm(ImGui::GetMousePos()) - CurInputPortPos);
        if (Distance < HoverRadius)
        {
            __setHoveredItem(vNodeId, EItemType::PORT, InputName, true);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) // start adding link
            {
                m_IsAddingLink = true;
                m_FixedPort = { vNodeId , InputName };
                m_IsFixedPortSource = false;
            }
        }
        
        if (m_IsAddingLink && m_IsFixedPortSource && m_FixedPort->NodeId != vNodeId) // if attached to the adding link
        {
            // FIXME: too messy code!!! fix this!
            bool HasThisLink = false;
            SRenderPassGraphLink Link = {m_FixedPort.value(), { vNodeId, InputName } };
            
            for (const auto& Pair : m_pGraph->LinkMap)
            {
                if (Pair.second == Link)
                {
                    HasThisLink = true;
                    break;
                }
            }

            if (!HasThisLink)
            {
                if (Distance < m_AttachMinDistance && (!m_AttachedPort.has_value() || Distance < m_AttachedPortDistance))
                {
                    m_AttachedPortDistance = Distance;
                    m_AttachedPort = { vNodeId, InputName };
                }
            }
        }

        // if is hovered
        if (__isItemHovered(vNodeId, EItemType::PORT, InputName, true))
        {
            Radius = NODE_SLOT_HOVER_RADIUS;
            Color = IM_COL32(200, 200, 200, 200);
        }


        m_NodePortPosMap[vNodeId].Input[InputName] = CurInputPortPos;
        pDrawList->AddCircleFilled(__toImgui(CurInputPortPos), Radius, Color);

        ImVec2 TextSize = ImGui::CalcTextSize(InputName.c_str());
        pDrawList->AddText(__toImgui(CurInputPortPos + glm::vec2(NODE_SLOT_HOVER_RADIUS + 4.0f, -TextSize.y * 0.5)), PortTextColor, InputName.c_str());

        CurInputPortPos.y += PortItemHeight;
    }

    for (const auto& OutputName : vioNode.OutputSet)
    {

        float Radius = NODE_SLOT_RADIUS;
        unsigned Color = IM_COL32(150, 150, 150, 150);

        const float HoverRadius = NODE_SLOT_HOVER_RADIUS + 2.0f;
        const float Distance = glm::length(__toGlm(ImGui::GetMousePos()) - CurOutputPortPos);
        if (Distance < HoverRadius)
        {
            __setHoveredItem(vNodeId, EItemType::PORT, OutputName, false);

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_IsAddingLink = true;
                m_FixedPort = { vNodeId , OutputName };
                m_IsFixedPortSource = true;
            }
        }
        
        if (m_IsAddingLink && !m_IsFixedPortSource && m_FixedPort->NodeId != vNodeId) // if attached to the adding link
        {
            // FIXME: too messy code!!! fix this!
            bool HasThisLink = false;
            SRenderPassGraphLink Link = { { vNodeId, OutputName }, m_FixedPort.value() };

            for (const auto& Pair : m_pGraph->LinkMap)
            {
                if (Pair.second == Link)
                {
                    HasThisLink = true;
                    break;
                }
            }

            if (!HasThisLink)
            {
                if (Distance < m_AttachMinDistance && (!m_AttachedPort.has_value() || Distance < m_AttachedPortDistance))
                {
                    m_AttachedPortDistance = Distance;
                    m_AttachedPort = { vNodeId, OutputName };
                }
            }
        }

        // if is hovered
        if (__isItemHovered(vNodeId, EItemType::PORT, OutputName, false))
        {
            Radius = NODE_SLOT_HOVER_RADIUS;
            Color = IM_COL32(200, 200, 200, 200);
        }
        
        m_NodePortPosMap[vNodeId].Output[OutputName] = CurOutputPortPos;
        pDrawList->AddCircleFilled(__toImgui(CurOutputPortPos), Radius, Color);

        ImVec2 TextSize = ImGui::CalcTextSize(OutputName.c_str());
        pDrawList->AddText(__toImgui(CurOutputPortPos + glm::vec2(-NODE_SLOT_HOVER_RADIUS - 4.0f - TextSize.x, -TextSize.y * 0.5)), PortTextColor, OutputName.c_str());

        CurOutputPortPos.y += PortItemHeight;
    }

    ImGui::PopID();
}

void CRenderPassGraphUI::__drawCurveAnimation(const Math::SCubicBezier2D& vCurve, unsigned vColor, float vRadius)
{
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
   
    const int CircleNum = 4;
    float Duration = 0.5f;
    float Interval = 1.0f / CircleNum;
    float Shift = glm::mod(m_AnimationTime / Duration, 1.0f) * Interval;
    for (int i = 0; i < CircleNum; ++i)
    {
        float t = i * Interval + Shift;
        float Alpha = (t < 0.5 ? t : 1.0 - t) * 2; // fade in-out
        Alpha = Math::smoothstepInversed(Alpha); // smooth
        unsigned char Alpha8Bit = unsigned char(Alpha * 255);
        unsigned CircleColor = (vColor & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, Alpha8Bit);
        glm::vec2 Pos = vCurve.sample(t);
        pDrawList->AddCircleFilled(__toImgui(Pos), vRadius, CircleColor);
    }
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

            float ForceOn1 = glm::min(300.0f / (d * d), 300.0f / m_WorldScale);
            ForceMap[Id1] += ForceOn1 * ForceOn1Direction;
            ForceMap[Id2] -= ForceOn1 * ForceOn1Direction;
        }
    }

    // 2. link attraction
    for (const auto& Pair : m_pGraph->LinkMap)
    {
        const SRenderPassGraphLink& Link = Pair.second;
        glm::vec2 v = (m_pGraph->NodeMap.at(Link.Destination.NodeId).getAABB().getCenter() - m_pGraph->NodeMap.at(Link.Source.NodeId).getAABB().getCenter()) * m_WorldScale;
        float d = glm::length(v);
        glm::vec2 ForceOn1Direction = d > 1e-3 ? glm::normalize(v) : glm::vec2(1, 0);

        float ForceOn1 = 5000.0f * d;
        glm::vec2 F = ForceOn1Direction * ForceOn1;
        ForceMap[Link.Source.NodeId] += F;
        ForceMap[Link.Destination.NodeId] -= F;
    }
        
    for (const auto& Pair : ForceMap)
    {
        size_t NodeId = Pair.first;
        if (__isItemSelected(NodeId, EItemType::NODE)) continue; // dragging
        glm::vec2 F = Pair.second;
        SRenderPassGraphNode& Node = m_pGraph->NodeMap.at(NodeId);
            
        glm::vec2 A = F / 1.0f;
        glm::vec2 dx = A * Step * Step;

        if (glm::length(dx) < 1e-2f) continue; // remove too small force to avoid flickering
        Node.Pos = Node.Pos + dx;
        _ASSERTE(Node.Pos.x != NAN && Node.Pos.y != NAN);
        _ASSERTE(Node.Pos.x != INFINITY && Node.Pos.y != INFINITY);
    }
}

namespace ImGui
{
    bool isAnyMouseClicked()
    {
        return ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
    }
}

void CRenderPassGraphUI::_renderUIV()
{
    // reset state
    m_HoveredItem.reset();
    m_AttachedPort.reset();

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
    for (const auto& Pair : m_pGraph->LinkMap)
    {
        __drawLink(Pair.first, Pair.second); // depend on renderer node, so draw after node
    }
    pDrawList->ChannelsMerge();

    // draw to add link
    if (m_IsAddingLink)
    {
        // TODO: complex and diffusing link info parsing...deal with this
        _ASSERTE(m_FixedPort.has_value());
        glm::vec2 MousePos = __toGlm(ImGui::GetMousePos());
        glm::vec2 AttchedPos = MousePos;
        if (m_AttachedPort.has_value())
        {
            if (m_IsFixedPortSource)
            {
                AttchedPos = m_NodePortPosMap[m_AttachedPort->NodeId].Input[m_AttachedPort->Name];
            }
            else
            {
                AttchedPos = m_NodePortPosMap[m_AttachedPort->NodeId].Output[m_AttachedPort->Name];
            }
        }

        glm::vec2 Start = m_IsFixedPortSource ? m_NodePortPosMap[m_FixedPort->NodeId].Output[m_FixedPort->Name] : AttchedPos;
        glm::vec2 End = !m_IsFixedPortSource ? m_NodePortPosMap[m_FixedPort->NodeId].Input[m_FixedPort->Name] : AttchedPos;
        Math::SCubicBezier2D LinkCurve = __createLinkCurve(Start, End);

        auto LinkColor = IM_COL32(200, 150, 150, 255);
        float LinkThickness = 6.0f;
        __drawLinkCurve(LinkCurve, LinkColor, LinkThickness);

        // if matched, draw animation
        if (m_AttachedPort.has_value())
            __drawCurveAnimation(LinkCurve, LinkColor, LinkThickness + 2.0f);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (m_AttachedPort.has_value()) // add a link
            {
                if (m_IsFixedPortSource)
                {
                    m_Editor.addLink(m_FixedPort.value(), m_AttachedPort.value());
                }
                else
                {
                    m_Editor.addLink(m_AttachedPort.value(), m_FixedPort.value());
                }
            }

            m_FixedPort.reset();
            m_AttachedPort.reset();
            m_IsAddingLink = false;
        }
    }

    // state
    if ((ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        && !m_HoveredItem.has_value() && !ImGui::IsAnyItemHovered())
    {
        m_SelectedItem.reset();
    }

    // context menu
    if (ImGui::isAnyMouseClicked())
    {
        m_IsContextMenuOpen = false; // close on click
    }
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && m_HoveredItem.has_value() && m_HoveredItem->Type != EItemType::PORT)
    {
        m_IsContextMenuOpen = true;
        m_SelectedItem = m_HoveredItem;
        m_DeferSelectedItem = m_HoveredItem;
    }

    if (m_IsContextMenuOpen)
    {
        ImGui::OpenPopup("NodeContentMenu");
    }

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("NodeContentMenu"))
    {
        if (m_DeferSelectedItem.has_value())
        {
            if (m_DeferSelectedItem->Type == EItemType::NODE)
            {
                SRenderPassGraphNode& Node = m_pGraph->NodeMap.at(m_DeferSelectedItem->Id);
                if (ImGui::MenuItem((u8"删除节点 " + Node.Name).c_str(), nullptr, false, true))
                {
                    m_Editor.removeNode(m_DeferSelectedItem->Id);
                    m_IsContextMenuOpen = false;
                    m_DeferSelectedItem.reset();
                    m_HoveredItem.reset();
                }
            }
            else if (m_DeferSelectedItem->Type == EItemType::LINK)
            {
                if (ImGui::MenuItem(u8"删除链接", nullptr, false, true))
                {
                    m_Editor.removeLink(m_DeferSelectedItem->Id);
                    m_IsContextMenuOpen = false;
                    m_DeferSelectedItem.reset();
                    m_HoveredItem.reset();
                }
            }
            else
            {
                _SHOULD_NOT_GO_HERE;
            }
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
                size_t NodeId = Pair.first;
                const SRenderPassGraphNode& Node = Pair.second;
                ImGui::PushID(NodeId);
                if (ImGui::Selectable(Node.Name.c_str(), __isItemSelected(NodeId, EItemType::NODE)))
                {
                    __setSelectedItem(NodeId, EItemType::NODE);
                }
                if (ImGui::IsItemHovered())
                {
                    __setHoveredItem(NodeId, EItemType::NODE);
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

    // update defer
    m_DeferHoveredItem = m_HoveredItem;
    m_DeferSelectedItem = m_SelectedItem;

    m_AnimationTime += m_Timer.tick();
}

bool CRenderPassGraphUI::__isItemSelected(size_t vId, EItemType vType, const std::string& vName, bool vIsInput) const
{
    return m_DeferSelectedItem.has_value() && m_DeferSelectedItem->Type == vType && m_DeferSelectedItem->Id == vId &&
        (m_DeferSelectedItem->Type != EItemType::PORT || (m_DeferSelectedItem->IsInput == vIsInput && m_DeferSelectedItem->Name == vName));
}

void CRenderPassGraphUI::__setSelectedItem(size_t vId, EItemType vType, const std::string& vName, bool vIsInput)
{
    m_SelectedItem = { vType, vId, vName, vIsInput };
}

bool CRenderPassGraphUI::__isItemHovered(size_t vId, EItemType vType, const std::string& vName, bool vIsInput) const
{
    return m_DeferHoveredItem.has_value() && m_DeferHoveredItem->Type == vType && m_DeferHoveredItem->Id == vId &&
        (m_DeferHoveredItem->Type != EItemType::PORT || (m_DeferHoveredItem->IsInput == vIsInput && m_DeferHoveredItem->Name == vName));
}

void CRenderPassGraphUI::__setHoveredItem(size_t vId, EItemType vType, const std::string& vName, bool vIsInput)
{
    m_HoveredItem = { vType, vId, vName, vIsInput };
}
