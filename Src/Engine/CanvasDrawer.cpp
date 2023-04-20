#include "CanvasDrawer.h"

ImVec2 __toImgui(glm::vec2 v) { return ImVec2(v.x, v.y); }

void CCanvasDrawer::setCanvasInfo(const glm::vec2& vPos, const glm::vec2& vSize)
{
    m_CanvasPos = vPos;
    m_CanvasSize = vSize;
}

glm::vec2 CCanvasDrawer::applyTransform(const glm::vec2& vWorld)
{
    return __toScreen(vWorld);
}

glm::vec2 CCanvasDrawer::applyScale(const glm::vec2& vWorld)
{
    return vWorld * m_Scale;
}

float CCanvasDrawer::applyScale(float vValue)
{
    return vValue * m_Scale;
}

glm::vec2 CCanvasDrawer::inverseTransform(const glm::vec2& vScreen)
{
    return (vScreen - m_Offset - m_CanvasPos) / m_Scale;
}

void ::CCanvasDrawer::begin()
{
    m_pDrawList = ImGui::GetWindowDrawList();
}

void CCanvasDrawer::drawLine(const glm::vec2& vStart, const glm::vec2& vEnd, ImColor vColor, float vThickness)
{
    m_pDrawList->AddLine(__toScreenImgui(vStart), __toScreenImgui(vEnd), vColor, vThickness);
}

void CCanvasDrawer::drawOutlineRect(const glm::vec2& vStart, const glm::vec2& vSize, ImColor vColor, float vRounding, ImDrawCornerFlags vFlags)
{
    m_pDrawList->AddRect(__toScreenImgui(vStart), __toScreenImgui(vStart + vSize), vColor, vRounding, vFlags);
}

void CCanvasDrawer::drawSolidRect(const glm::vec2& vStart, const glm::vec2& vSize, ImColor vColor, float vRounding, ImDrawCornerFlags vFlags)
{
    m_pDrawList->AddRectFilled(__toScreenImgui(vStart), __toScreenImgui(vStart + vSize), vColor, vRounding, vFlags);
}

void CCanvasDrawer::drawSolidCircle(const glm::vec2& vCenter, float vRadius, ImColor vColor)
{
    m_pDrawList->AddCircleFilled(__toScreenImgui(vCenter), vRadius, vColor);
}

void CCanvasDrawer::drawText(const glm::vec2& vPos, const std::string& vText, ImColor vColor)
{

    m_pDrawList->AddText(__toScreenImgui(vPos), vColor, vText.c_str());
}

void CCanvasDrawer::drawBezier(const Math::SCubicBezier2D& vBezier, ImColor vColor, float vThickness)
{
    m_pDrawList->AddBezierCurve(
        __toScreenImgui(vBezier.Points[0]),
        __toScreenImgui(vBezier.Points[1]),
        __toScreenImgui(vBezier.Points[2]),
        __toScreenImgui(vBezier.Points[3]),
        vColor, vThickness);
}

void CCanvasDrawer::drawGrid(float vGridSize, ImColor vColor, float vThickness)
{
    float AcutalGridSize = vGridSize * m_Scale;
    for (float x = fmodf(m_Offset.x, AcutalGridSize); x < m_CanvasSize.x; x += AcutalGridSize)
    {
        m_pDrawList->AddLine(__toImgui(glm::vec2(x, 0.0f) + m_CanvasPos), __toImgui(glm::vec2(x, m_CanvasSize.y) + m_CanvasPos), vColor, vThickness);
    }
    for (float y = fmodf(m_Offset.y, AcutalGridSize); y < m_CanvasSize.y; y += AcutalGridSize)
    {
        m_pDrawList->AddLine(__toImgui(glm::vec2(0.0f, y) + m_CanvasPos), __toImgui(glm::vec2(m_CanvasSize.x, y) + m_CanvasPos), vColor, vThickness);
    }}

bool CCanvasDrawer::addInvisibleButton(const glm::vec2& vPos, const glm::vec2& vSize)
{
    ImGui::SetCursorScreenPos(__toScreenImgui(vPos));
    return ImGui::InvisibleButton("node", __toImgui(applyScale(vSize)));
}

void CCanvasDrawer::beginLayerSplitting(int vLayerNum)
{
    _ASSERTE(!m_IsLayerSplittingBeginned);
    m_LayerNum = vLayerNum;
    m_IsLayerSplittingBeginned = true;
    m_pDrawList->ChannelsSplit(m_LayerNum);
}

void CCanvasDrawer::switchLayer(int vLayerIndex)
{
    _ASSERTE(m_IsLayerSplittingBeginned);
    _ASSERTE(0 <= vLayerIndex && vLayerIndex < m_LayerNum);
    m_pDrawList->ChannelsSetCurrent(vLayerIndex);
}

void CCanvasDrawer::endLayerSplitting()
{
    _ASSERTE(m_IsLayerSplittingBeginned);
    m_LayerNum = 0;
    m_IsLayerSplittingBeginned = false;
    m_pDrawList->ChannelsMerge();
}

void CCanvasDrawer::setCursor(const glm::vec2& vWorld)
{
    ImGui::SetCursorScreenPos(__toScreenImgui(vWorld));
}

glm::vec2 CCanvasDrawer::__toScreen(const glm::vec2& vWorld) const
{
    return vWorld * m_Scale + m_Offset + m_CanvasPos;
}

ImVec2 CCanvasDrawer::__toScreenImgui(const glm::vec2& vWorld) const
{
    return __toImgui(__toScreen(vWorld));
}
