#include "RenderPassGraphUI.h"
#include "Maths.h"
#include "RenderpassLib.h"
#include "NativeSystem.h"
#include "RenderPassGraphIO.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "InterfaceGui.h"

// reference: https://gist.github.com/ocornut/7e9b3ec566a333d725d4

// parameters
namespace
{
    ImU32 gGridColor = IM_COL32(200, 200, 200, 40);
    float gGridSize = 64.0f;
    float gGridThickness = 1.0f;

    float gNodeWidth = 120.0f; // dynamic calculating ti avoid overlap or too empty
    float gNodeRounding = 4.0f;
    float gNodeHeaderPadding = 8.0f;
    ImU32 gNodeHeaderBackgroundColor = IM_COL32(80, 0, 0, 255);
    ImU32 gNodeHeaderTextColor = IM_COL32(255, 255, 255, 255);

    float gPortRadius = 8.0f;
    float gPortMargin = 8.0f;
    ImU32 gPortTextColor = IM_COL32(255, 255, 255, 255);
    float gPortHoveredRadius = gPortRadius + 2.0f;
    ImU32 gPortColor = IM_COL32(150, 150, 150, 150);
    ImU32 gPortHoveredColor = IM_COL32(200, 200, 200, 200);
    float gPortHoverDistance = gPortHoveredRadius;
    float gPortAttachMinDistance = gPortHoverDistance;

    ImU32 gLinkColor = IM_COL32(200, 200, 100, 255);
    float gLinkThickness = 3.0f;
    ImU32 gLinkHoveredColor = IM_COL32(255, 255, 150, 255);
    float gLinkHoveredThickness = 6.0f;
    float gLinkCurveGradient = 50.0f;
    float gLinkHoverDistance = 6.0f;

    float gLinkAnimeCircleRadius = gLinkHoveredThickness + 2.0f;
    int gLinkAnimeCircleNum = 4;
    float gLinkAnimeDuration = 0.5f;

    float gEntryLineLength = 10.0f;
    float gEntryLineWidth = 2.0f;
    float gEntryLineMargin = 4.0f;
    ImU32 gEntryLineColor = ImColor(255, 200, 200, 255);
    ImU32 gEntryTextColor = ImColor(255, 200, 200, 255);

    const int gSidebarWidth = 200;

    glm::vec2 __toGlm(ImVec2 v) { return glm::vec2(v.x, v.y); }
}

namespace ImGui
{
    bool isAnyMouseClicked()
    {
        return ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
    }
}

Math::SCubicBezier2D __createLinkCurve(const glm::vec2& vStart, const glm::vec2& vEnd)
{
    glm::vec2 P2 = vStart + glm::vec2(gLinkCurveGradient, 0);
    glm::vec2 P3 = vEnd + glm::vec2(-gLinkCurveGradient, 0);
    return Math::SCubicBezier2D(vStart, P2, P3, vEnd);
}

void CRenderPassGraphUI::__drawLink(size_t vLinkId, const SRenderPassGraphLink& vLink)
{
    glm::vec2 Start = __getPortPos(vLink.Source, false);
    glm::vec2 End = __getPortPos(vLink.Destination, true);
    Math::SCubicBezier2D LinkCurve = __createLinkCurve(Start, End);

    if (!m_IsContextMenuOpen)
    {
        glm::vec2 MousePos = m_CanvasDrawer.getMousePosInWorld();
        float d = LinkCurve.calcDistanceToPoint(MousePos);
        if (d <= m_CanvasDrawer.inverseScale(gLinkHoverDistance)  && !m_HoveredItem.has_value()) // only when no node hovered
        {
            __setHoveredItem(vLinkId, EItemType::LINK);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                __setSelectedItem(vLinkId, EItemType::LINK);
            }
        }
    }

    auto LinkColor = gLinkColor;
    float LinkThickness = gLinkThickness;
    bool Hovered = __isItemHovered(vLinkId, EItemType::LINK);
    if (Hovered)
    {
        LinkColor = gLinkHoveredColor;
        LinkThickness = gLinkHoveredThickness;
    }

    m_CanvasDrawer.drawBezier(LinkCurve, LinkColor, LinkThickness);

    // draw link animation
    unsigned BaseCircleColor = LinkColor;
    if (__isItemSelected(vLinkId, EItemType::LINK))
        __drawCurveAnimation(LinkCurve, BaseCircleColor, gLinkAnimeCircleRadius);
}

void CRenderPassGraphUI::__drawNode(size_t vNodeId, SRenderPassGraphNode& vioNode)
{
    const float LineHeight = ImGui::GetTextLineHeight();
    const float HeaderHeight = LineHeight + gNodeHeaderPadding;
    const float PortItemHeight = glm::max(ImGui::GetTextLineHeight(), gPortHoveredRadius) + gPortMargin;

    ImGuiIO& io = ImGui::GetIO();

    const glm::vec2 NodePos = vioNode.Pos;

    float ContentPadding = 8.0f;
    float ContentHeight = PortItemHeight * glm::max(__getNodeInputs(vNodeId).size(), __getNodeOutputs(vNodeId).size()) + ContentPadding * 2;

    glm::vec2 NodeSize = glm::vec2(gNodeWidth, HeaderHeight + ContentHeight);
    m_NodeSizeMap[vNodeId] = NodeSize;

    ImGui::PushID(static_cast<int>(vNodeId));

    // draw invisible button for interaction
    m_CanvasDrawer.switchLayer(0);
    m_CanvasDrawer.addInvisibleButton(NodePos, NodeSize);
    if (!m_IsContextMenuOpen && ImGui::IsItemHovered())
    {
        __setHoveredItem(vNodeId, EItemType::NODE);
    }

    // dragging
    if (!m_AddLinkState.isStarted())
    {
        bool IsMoving = ImGui::IsItemActive();
        if (IsMoving)
            __setSelectedItem(vNodeId, EItemType::NODE);
        if (IsMoving && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            vioNode.Pos = vioNode.Pos + __toGlm(io.MouseDelta) / m_CanvasDrawer.getScale();
    }

    // work on background
    m_CanvasDrawer.switchLayer(1);

    // draw whole node background and boarder
    ImU32 BackgroundColor = IM_COL32(60, 60, 60, 255);
    if (__isItemSelected(vNodeId, EItemType::NODE))
        BackgroundColor = IM_COL32(100, 100, 100, 255);
    else if (__isItemHovered(vNodeId, EItemType::NODE))
        BackgroundColor = IM_COL32(75, 75, 75, 255);
    m_CanvasDrawer.drawSolidRect(NodePos, NodeSize, BackgroundColor, gNodeRounding);
    m_CanvasDrawer.drawOutlineRect(NodePos, NodeSize, IM_COL32(100, 100, 100, 255), gNodeRounding);

    // draw header background
    m_CanvasDrawer.drawSolidRect(NodePos, glm::vec2(gNodeWidth, HeaderHeight), gNodeHeaderBackgroundColor, gNodeRounding, ImDrawFlags_RoundCornersTop);

    // draw header texts
    std::string NodeName = __getNodeName(vNodeId);
    m_CanvasDrawer.switchLayer(2); // Foreground
    m_CanvasDrawer.drawText(NodePos + glm::vec2(gNodeHeaderPadding, HeaderHeight * 0.5f), NodeName, gNodeHeaderTextColor, CCanvasDrawer::ETextAlign::BEGIN, CCanvasDrawer::ETextAlign::CENTER);

    // draw ports
    m_NodePortPosMap[vNodeId] = SPortPos();
    glm::vec2 InputPortStartPos = NodePos + glm::vec2(0.0f, HeaderHeight + ContentPadding + PortItemHeight * 0.5);
    glm::vec2 OutputPortStartPos = InputPortStartPos + glm::vec2(gNodeWidth, 0.0f);

    __drawPorts(vNodeId, InputPortStartPos, PortItemHeight, true);
    __drawPorts(vNodeId, OutputPortStartPos, PortItemHeight, false);
    ImGui::PopID();
}

void CRenderPassGraphUI::__drawCurveAnimation(const Math::SCubicBezier2D& vCurve, unsigned vColor, float vRadius)
{
    float Interval = 1.0f / gLinkAnimeCircleNum;
    float Shift = glm::mod(m_AnimationTime / gLinkAnimeDuration, 1.0f) * Interval;
    for (int i = 0; i < gLinkAnimeCircleNum; ++i)
    {
        float t = i * Interval + Shift;
        float Alpha = (t < 0.5f ? t : 1.0f - t) * 2.0f; // fade in-out
        Alpha = Math::smoothstepInversed(Alpha); // smooth
        unsigned char Alpha8Bit = unsigned char(Alpha * 255);
        unsigned CircleColor = (vColor & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, Alpha8Bit);
        glm::vec2 Pos = vCurve.sample(t);
        m_CanvasDrawer.drawSolidCircle(Pos, vRadius, CircleColor);
    }
}

void CRenderPassGraphUI::__drawPorts(size_t vNodeId, glm::vec2 vStartPos, float vPortItemHeight, bool vIsInput)
{
    auto& PortPos = m_NodePortPosMap.at(vNodeId);
    const glm::vec2 MousePos = m_CanvasDrawer.getMousePosInWorld();
    const bool IsSource = !vIsInput;

    glm::vec2 CurPos = vStartPos;
    auto& PortPosMap = vIsInput ? PortPos.Input : PortPos.Output;
    const auto& PortSet = vIsInput ? __getNodeInputs(vNodeId) : __getNodeOutputs(vNodeId);
    for (const auto& PortName : PortSet)
    {
        PortPosMap[PortName] = CurPos;

        float Radius = gPortRadius;
        unsigned Color = gPortColor;

        const float Distance = glm::length(MousePos - CurPos);
        if (Distance < m_CanvasDrawer.inverseScale(gPortHoverDistance))
        {
            __setHoveredItem(vNodeId, EItemType::PORT, PortName, vIsInput);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) // start adding link
            {
                m_AddLinkState.start({ vNodeId , PortName }, IsSource);
            }
        }

        if (m_AddLinkState.isStarted() && Distance < gPortAttachMinDistance) // if attached to the adding link
        {
            m_AddLinkState.addCandidate({ vNodeId, PortName }, IsSource, -Distance);
        }

        // if is hovered
        if (__isItemHovered(vNodeId, EItemType::PORT, PortName, vIsInput))
        {
            Radius = gPortHoveredRadius;
            Color = gPortHoveredColor;
        }

        m_CanvasDrawer.drawSolidCircle(CurPos, Radius, Color);

        glm::vec2 TextPos = CurPos;
        CCanvasDrawer::ETextAlign HorizonAlign = CCanvasDrawer::ETextAlign::BEGIN;
        if (vIsInput)
            TextPos.x += m_CanvasDrawer.inverseScale(gPortHoveredRadius) + 4.0f;
        else
        {
            TextPos.x -= m_CanvasDrawer.inverseScale(gPortHoveredRadius) + 4.0f;
            HorizonAlign = CCanvasDrawer::ETextAlign::END;
        }
        m_CanvasDrawer.drawText(TextPos, PortName, gPortTextColor, HorizonAlign, CCanvasDrawer::ETextAlign::CENTER);

        CurPos.y += vPortItemHeight;
    }
}

void CRenderPassGraphUI::update()
{
    if (!m_EnableForce) return;
    const float dpi = 300.0f;
    const float m_WorldScale = 1.0f / dpi; // dpi

    // force graph
    float Step = 0.01f;
    std::map<size_t, glm::vec2> ForceMap;

    // 1. node repulsion
    for (auto pIter1 = m_pGraph->NodeMap.begin(); pIter1 != m_pGraph->NodeMap.end(); ++pIter1)
    {
        size_t Id1 = pIter1->first;
        Math::SAABB2D NodeAABB1 = __getNodeBound(Id1);
        for (auto pIter2 = std::next(pIter1); pIter2 != m_pGraph->NodeMap.end(); ++pIter2)
        {
            size_t Id2 = pIter2->first;
            Math::SAABB2D NodeAABB2 = __getNodeBound(Id2);

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
        glm::vec2 v = (__getNodeBound(Link.Destination.NodeId).getCenter() - __getNodeBound(Link.Source.NodeId).getCenter()) * m_WorldScale;
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

void CRenderPassGraphUI::_renderUIV()
{
    if (!UI::beginWindow("Render Pass Graph", nullptr, UI::EWindowFlag::MENU_BAR)) return;

    // reset state
    m_HoveredItem.reset();
    if (m_AddLinkState.isStarted())
        m_AddLinkState.clearCandidates();

    // menu
    if (UI::beginMenuBar())
    {
        if (UI::beginMenu(u8"文件"))
        {
            if (UI::menuItem(u8"保存到"))
            {
                auto SaveDialogResult = Gui::createSaveFileDialog("graph");
                if (SaveDialogResult.Action)
                {
                    RenderPassGraphIO::save(m_pGraph, SaveDialogResult.FilePath);
                }
            }
            UI::endMenu();
        }
        if (UI::beginMenu(u8"编辑"))
        {
            if (UI::menuItem(u8"撤销", nullptr, m_Editor.canUndo()))
                m_Editor.undo();
            if (UI::menuItem(u8"重做", nullptr, m_Editor.canRedo()))
                m_Editor.redo();
            UI::endMenu();
        }
        if (UI::beginMenu(u8"设置"))
        {
            UI::menuItem(u8"显示网格", &m_ShowGrid);
            UI::menuItem(u8"开启节点引力", &m_EnableForce);
            UI::endMenu();
        }
        UI::endMenuBar();
    }

    ImGui::BeginChild("Visualize", ImVec2(-gSidebarWidth, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
    UI::beginGroup();
    // Create our child canvas
    float CanvasScale = m_CanvasDrawer.getScale();
    glm::vec2 CanvasCenter = m_CanvasDrawer.getOffset() / CanvasScale;
    glm::vec2 MousePos = m_CanvasDrawer.getMousePosInWorld();
    ImGui::Text(u8"中心 (%7.2f,%7.2f), 缩放 %4.2f, 鼠标位置 (%7.2f,%7.2f)", CanvasCenter.x, CanvasCenter.y, CanvasScale, MousePos.x, MousePos.y);

    if (UI::button(u8"重置视角"))
        resetView(50.0f);

    UI::sameLine();
    
    std::string ApplyButtonText = m_pGraph->isValid() ? u8"应用更改" : u8"无法应用更改：渲染图无效";
    ImGui::BeginDisabled(!m_pGraph->isValid());
    if (UI::button(ApplyButtonText))
    {
        m_GraphApplyEventHandler.trigger(m_pGraph);
    }
    ImGui::EndDisabled();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
    ImGui::BeginChild("Canvas", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(); // WindowPadding

    // canvas
    m_CanvasDrawer.begin();
    m_CanvasDrawer.setCanvasInfo(__toGlm(ImGui::GetCursorScreenPos()), __toGlm(ImGui::GetWindowSize()));

    // draw grid
    if (m_ShowGrid)
        m_CanvasDrawer.drawGrid(gGridSize, gGridColor, gGridThickness);
    
    m_CanvasDrawer.beginLayerSplitting(3);

    // draw nodes
    for (auto& Pair : m_pGraph->NodeMap)
    {
        size_t Id = Pair.first;
        SRenderPassGraphNode& Node = Pair.second;
        __drawNode(Id, Node);
    }

    // draw links
    m_CanvasDrawer.switchLayer(0); // Background
    for (const auto& Pair : m_pGraph->LinkMap)
    {
        __drawLink(Pair.first, Pair.second); // depend on renderer node, so draw after node
    }
    m_CanvasDrawer.endLayerSplitting();

    // draw output
    if (m_pGraph->OutputPort.has_value())
    {
        glm::vec2 PortPos = __getPortPos(m_pGraph->OutputPort.value(), false);
        
        glm::vec2 LineStart = PortPos + glm::vec2(gEntryLineLength, 0);
        glm::vec2 LineEnd = LineStart;
        LineEnd.x += gEntryLineMargin + m_CanvasDrawer.inverseScale(gPortHoveredRadius);
        m_CanvasDrawer.drawLine(LineStart, LineEnd, gEntryLineColor, gEntryLineWidth);

        std::string OutputText = u8"输出";
        glm::vec2 TextPos = LineEnd;
        TextPos.x += 4.0f;
        m_CanvasDrawer.drawText(TextPos, OutputText, gEntryTextColor, CCanvasDrawer::ETextAlign::BEGIN, CCanvasDrawer::ETextAlign::CENTER);
    }

    // draw to add link
    if (m_AddLinkState.isStarted())
    {
        std::string Reason = "";
        EAddLinkAttachState AttachState = m_AddLinkState.getLinkState(Reason);

        const auto& FixedPort = m_AddLinkState.getFixedPort();
        glm::vec2 Start = __getPortPos(FixedPort, !m_AddLinkState.isFixedPortSource());

        glm::vec2 MousePos = m_CanvasDrawer.getMousePosInWorld();
        glm::vec2 End = MousePos;

        if (AttachState == EAddLinkAttachState::VALID_ATTACH)
        {
            const auto& AttachedPort = m_AddLinkState.getCurrentAttachedPort();
            End = __getPortPos(AttachedPort, m_AddLinkState.isFixedPortSource());
        }


        if (!m_AddLinkState.isFixedPortSource()) // swap direction
        {
            std::swap(Start, End);
        }

        Math::SCubicBezier2D LinkCurve = __createLinkCurve(Start, End);

        auto LinkColor = IM_COL32(200, 200, 200, 255);
        if (AttachState == EAddLinkAttachState::VALID_ATTACH)
            LinkColor = IM_COL32(255, 255, 150, 255);
        else if (AttachState == EAddLinkAttachState::INVALID_ATTACH)
            LinkColor = IM_COL32(200, 50, 50, 255);

        float LinkThickness = 6.0f;
        m_CanvasDrawer.drawBezier(LinkCurve, LinkColor, LinkThickness);

        if (AttachState == EAddLinkAttachState::VALID_ATTACH) // if matched, draw animation
        {
            __drawCurveAnimation(LinkCurve, LinkColor, LinkThickness + 2.0f);
        }
        else if (AttachState == EAddLinkAttachState::INVALID_ATTACH) // if invalid, show why
        {
            glm::vec2 Center = LinkCurve.sample(0.5f);
            glm::vec2 TextSize = m_CanvasDrawer.calcActualTextSize(Reason);
            float BgPadding = 4.0f;
            unsigned BgColor = IM_COL32(100, 100, 100, 200);
            glm::vec2 BgSize = TextSize + 2.0f * BgPadding;
            m_CanvasDrawer.drawSolidRect(Center - BgSize * 0.5f, BgSize, BgColor, 4.0f);
            m_CanvasDrawer.drawText(Center, Reason, IM_COL32(255, 150, 150, 255), CCanvasDrawer::ETextAlign::CENTER, CCanvasDrawer::ETextAlign::CENTER);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (AttachState == EAddLinkAttachState::VALID_ATTACH)
            {
                const SRenderPassGraphLink& NewLink = m_AddLinkState.getCurrentValidLink();
                m_Editor.addLink(NewLink);
            }
            m_AddLinkState.end();
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

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && m_HoveredItem.has_value() &&
        (m_HoveredItem->Type != EItemType::PORT || !m_HoveredItem->IsInput))
    {
        m_IsContextMenuOpen = true;
        m_SelectedItem = m_HoveredItem;
        m_DeferSelectedItem = m_HoveredItem;
    }

    if (m_IsContextMenuOpen)
    {
        ImGui::OpenPopup("NodeContentMenu");
    }
    
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
            else if (m_DeferSelectedItem->Type == EItemType::PORT)
            {
                
                _ASSERTE(!m_DeferSelectedItem->IsInput);
                if (ImGui::MenuItem(u8"设为输出", nullptr, false, true))
                {
                    m_Editor.setEntry(m_DeferSelectedItem->Id, m_DeferSelectedItem->Name);
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

    // Panning and scaling
    const ImGuiIO& IO = ImGui::GetIO();
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
            m_CanvasDrawer.setOffset(m_CanvasDrawer.getOffset() + __toGlm(IO.MouseDelta));

        if (glm::abs(IO.MouseWheel) > 0.1f)
        {
            // scaling anchor is mouse pos
            float NewScale = glm::max(0.1f, m_CanvasDrawer.getScale() + IO.MouseWheel * 0.1f);
            glm::vec2 MouseWorldPos = m_CanvasDrawer.getMousePosInWorld();
            glm::vec2 NewOffset = MouseWorldPos * (m_CanvasDrawer.getScale() - NewScale) + m_CanvasDrawer.getOffset();

            m_CanvasDrawer.setScale(NewScale);
            m_CanvasDrawer.setOffset(NewOffset);
        }
    }
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::EndChild();

    ImGui::SameLine();

    // draw sidebar
    ImGui::BeginChild("sidebar", ImVec2(gSidebarWidth, 0));
    if (ImGui::BeginTabBar("sidebartab"))
    {
        if (ImGui::BeginTabItem(u8"当前"))
        {
            for (const auto& Pair : m_pGraph->NodeMap)
            {
                size_t NodeId = Pair.first;
                std::string NodeName = __getNodeName(NodeId);
                ImGui::PushID(static_cast<int>(NodeId));
                if (ImGui::Selectable(NodeName.c_str(), __isItemSelected(NodeId, EItemType::NODE)))
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

        if (ImGui::BeginTabItem(u8"库"))
        {
            const auto& PassSet = RenderpassLib::getAllVisiblePassNames();
            if (PassSet.empty())
                ImGui::Text(u8"未注册任何渲染Pass...");
            else
            {
                float LineHeight = ImGui::GetTextLineHeight() + 4.0f;
                glm::vec2 AddButtonSize = glm::vec2(LineHeight, LineHeight);
                for (const auto& Name : PassSet)
                {
                    ImGui::Text(Name.c_str());
                    ImGui::SameLine(ImGui::GetWindowWidth() - LineHeight - 4.0f);
                    if (UI::button(u8"+##" + Name, AddButtonSize))
                    {
                        Math::SAABB2D GraphBound = __getGraphBound();
                        float x = GraphBound.Max.x + 50.0f;
                        float y = GraphBound.getCenter().y;
                        m_Editor.addNode(Name, glm::vec2(x, y));
                        __resetViewNextFrame();
                    }
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

    // others
    if (m_NeedResetViewTimes > 0)
    {
        resetView();
        --m_NeedResetViewTimes;
    }
    UI::endWindow();
}

void CRenderPassGraphUI::setGraph(ptr<SRenderPassGraph> vGraph, bool vResetView)
{
    m_pGraph = vGraph;
    m_Editor.setGraph(vGraph);
    m_AddLinkState.setGraph(vGraph);

    m_NodePortPosMap.clear();
    m_NodeSizeMap.clear();

    m_NeedResetViewTimes = 1;

    destroyPasses();
}

void CRenderPassGraphUI::destroyPasses()
{
    for (const auto& Pair : m_PassInstanceMap)
        Pair.second->destroy();
    m_PassInstanceMap.clear();
}

void CRenderPassGraphUI::resetView(float vPadding)
{
    Math::SAABB2D AABB = __getGraphBound();

    glm::vec2 Center = AABB.getCenter();
    glm::vec2 Extent = AABB.getExtent() + vPadding * 2.0f;
    glm::vec2 CanvasSize = m_CanvasDrawer.getCanvasSize();
    float Scale = glm::min(CanvasSize.x / Extent.x, CanvasSize.y / Extent.y);
    m_CanvasDrawer.setScale(Scale);
    float ActualScale = m_CanvasDrawer.getScale();
    m_CanvasDrawer.setOffset(-Center * ActualScale);
}

glm::vec2 CRenderPassGraphUI::__getPortPos(const SRenderPassGraphPortInfo& vPort, bool vIsInput) const
{
    if (vIsInput)
        return m_NodePortPosMap.at(vPort.NodeId).Input.at(vPort.Name);
    else
        return m_NodePortPosMap.at(vPort.NodeId).Output.at(vPort.Name);
}

CPortSet::CPtr CRenderPassGraphUI::__getNodePortSet(size_t vNodeId)
{
    const SRenderPassGraphNode& Node = m_pGraph->NodeMap.at(vNodeId);
    if (m_PassInstanceMap.find(vNodeId) == m_PassInstanceMap.end())
    {
        auto pPass = RenderpassLib::createPass(Node.Name);
        pPass->createPortSet();
        m_PassInstanceMap[vNodeId] = pPass;
    }
    auto pPass = m_PassInstanceMap.at(vNodeId);
    auto pPortSet = pPass->getPortSet();
    _ASSERTE(pPortSet);
    return pPortSet;
}

std::vector<std::string> CRenderPassGraphUI::__getNodeInputs(size_t vNodeId)
{
    auto pPortSet = __getNodePortSet(vNodeId);

    std::vector<std::string> InputSet;
    for (size_t i = 0; i < pPortSet->getInputPortNum(); ++i)
    {
        InputSet.push_back(pPortSet->getInputPort(i)->getName());
    }
    return InputSet;
}

std::vector<std::string> CRenderPassGraphUI::__getNodeOutputs(size_t vNodeId)
{
    auto pPortSet = __getNodePortSet(vNodeId);

    std::vector<std::string> OutputSet;
    for (size_t i = 0; i < pPortSet->getOutputPortNum(); ++i)
    {
        OutputSet.push_back(pPortSet->getOutputPort(i)->getName());
    }
    return OutputSet;
}

Math::SAABB2D CRenderPassGraphUI::__getNodeBound(size_t vNodeId) const
{
    const auto& Node = m_pGraph->NodeMap.at(vNodeId);
    if (m_NodeSizeMap.find(vNodeId) == m_NodeSizeMap.end())
        return Math::SAABB2D(Node.Pos, Node.Pos);
    else
    {
        glm::vec2 Size = m_NodeSizeMap.at(vNodeId);
        return Math::SAABB2D(Node.Pos, Node.Pos + Size);
    }
}

Math::SAABB2D CRenderPassGraphUI::__getGraphBound() const
{
    if (!m_pGraph || m_pGraph->NodeMap.empty())
        return  Math::SAABB2D(glm::vec2(-200), glm::vec2(200));

    std::optional<Math::SAABB2D> AABB;
    for (const auto& Pair : m_pGraph->NodeMap)
    {
        const auto& Bound = __getNodeBound(Pair.first);
        if (AABB.has_value())
            AABB->applyUnion(Bound);
        else
            AABB = Bound;
    }
    return AABB.value();
}

std::string CRenderPassGraphUI::__getNodeName(size_t vNodeId) const
{
    const SRenderPassGraphNode& Node = m_pGraph->NodeMap.at(vNodeId);
    return std::to_string(vNodeId) + "_" + Node.Name;
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
