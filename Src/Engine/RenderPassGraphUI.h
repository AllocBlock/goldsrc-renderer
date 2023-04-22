#pragma once
#include "RenderPassGraph.h"
#include "RenderPass.h"
#include "RenderPassGraphEditor.h"
#include "Event.h"
#include "Timer.h"
#include "Maths.h"
#include "RenderPassGraphAddLinkState.h"
#include "CanvasDrawer.h"

#include <string>
#include <glm/glm.hpp>

class CRenderPassGraphUI : public IDrawableUI
{
    _DEFINE_EVENT(GraphApply, ptr<SRenderPassGraph>);
public:
    virtual void _renderUIV() override;

    void setContext(vk::CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo);

    void setGraph(ptr<SRenderPassGraph> vGraph);
    void update();
    void destroyPasses();

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

    void __drawLink(size_t vLinkId, const SRenderPassGraphLink& vLink);
    void __drawNode(size_t vNodeId, SRenderPassGraphNode& vioNode);
    void __drawCurveAnimation(const Math::SCubicBezier2D& vCurve, unsigned vColor, float vRadius);
    void __drawPorts(size_t vNodeId, glm::vec2 vStartPos, float vPortItemHeight, bool vIsInput);

    // TODO: how to manage these copied function?
    bool __isItemSelected(size_t vId, EItemType vType, const std::string& vName = "", bool vIsInput = true) const;
    void __setSelectedItem(size_t vId, EItemType vType, const std::string& vName = "", bool vIsInput = true);
    bool __isItemHovered(size_t vNodeId, EItemType vType, const std::string& vName = "", bool vIsInput = true) const;
    void __setHoveredItem(size_t vId, EItemType vType, const std::string& vName = "", bool vIsInput = true);

    glm::vec2 __getPortPos(const SRenderPassGraphPortInfo& vPort, bool vIsInput) const;

    CPortSet::CPtr __getNodePortSet(size_t vNodeId);
    // TODO: cache to avoid redundant creation
    std::vector<std::string> __getNodeInputs(size_t vNodeId);
    std::vector<std::string> __getNodeOutputs(size_t vNodeId);

    vk::CDevice::CPtr m_pDevice = nullptr;
    CAppInfo::Ptr m_pAppInfo = nullptr;
    ptr<SRenderPassGraph> m_pGraph = nullptr;
    std::map<size_t, vk::IRenderPass::Ptr> m_PassInstanceMap;

    CCanvasDrawer m_CanvasDrawer;
    bool m_ShowGrid = true;
    bool m_IsContextMenuOpen = false;

    std::optional<SItemRef> m_HoveredItem = std::nullopt; // real-time update, override
    std::optional<SItemRef> m_DeferHoveredItem = std::nullopt; // actual hovered, defered for drawing
    std::optional<SItemRef> m_SelectedItem = std::nullopt;
    std::optional<SItemRef> m_DeferSelectedItem = std::nullopt;

    // temp data for drawing
    struct SPortPos
    {
        std::map<std::string, glm::vec2> Input;
        std::map<std::string, glm::vec2> Output;
    };
    std::map<size_t, SPortPos> m_NodePortPosMap;

    bool m_EnableForce = false;
    CTimer m_Timer;
    float m_AnimationTime = 0.0f;

    // edit
    CRenderGraphAddLinkState m_AddLinkState;
    CRenderPassGraphEditor m_Editor;
};