#pragma once
#include "Common.h"
#include "Maths.h"
#include <glm/glm.hpp>
#include <imgui.h>

class CCanvasDrawer
{
public:
    _DEFINE_GETTER_SETTER(Offset, glm::vec2);
    _DEFINE_GETTER_SETTER(Scale, float);

    void setCanvasInfo(const glm::vec2& vPos, const glm::vec2& vSize);

    glm::vec2 applyTransform(const glm::vec2& vWorld);
    glm::vec2 applyScale(const glm::vec2& vWorld);
    float applyScale(float vValue);
    glm::vec2 inverseTransform(const glm::vec2& vScreen);

    void begin();
    void drawLine(const glm::vec2& vStart, const glm::vec2& vEnd, ImColor vColor, float vThickness);
    void drawOutlineRect(const glm::vec2& vStart, const glm::vec2& vSize, ImColor vColor, float vRounding = 0.0f, ImDrawCornerFlags vFlags = 0);
    void drawSolidRect(const glm::vec2& vStart, const glm::vec2& vSize, ImColor vColor, float vRounding = 0.0f, ImDrawCornerFlags vFlags = 0);
    void drawSolidCircle(const glm::vec2& vCenter, float vRadius, ImColor vColor);
    void drawText(const glm::vec2& vPos, const std::string& vText, ImColor vColor);
    void drawBezier(const Math::SCubicBezier2D& vBezier, ImColor vColor, float vThickness);
    void drawGrid(float vGridSize, ImColor vColor, float vThickness);
    bool addInvisibleButton(const glm::vec2& vPos, const glm::vec2& vSize);

    void beginLayerSplitting(int vLayerNum);
    void switchLayer(int vLayerIndex);
    void endLayerSplitting();

    void setCursor(const glm::vec2& vWorld);

private:
    glm::vec2 __toScreen(const glm::vec2& vWorld) const;
    ImVec2 __toScreenImgui(const glm::vec2& vWorld) const;

    glm::vec2 m_CanvasPos = glm::vec2(0.0f);
    glm::vec2 m_CanvasSize = glm::vec2(0.0f);
    glm::vec2 m_Offset = glm::vec2(0.0f);
    float m_Scale = 1.0f;
    ImDrawList* m_pDrawList = nullptr;

    bool m_IsLayerSplittingBeginned = false;
    int m_LayerNum = 0;
};