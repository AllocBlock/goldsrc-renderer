#pragma once
#include "RenderPassGraph.h"
#include "RenderPass.h"
#include "RenderPassGraphEditor.h"
#include "Timer.h"
#include "Maths.h"

#include <string>
#include <vector>
#include <deque>
#include <glm/glm.hpp>

enum class EAddLinkAttachState
{
    NOT_ATTACHED,
    INVALID_ATTACH,
    VALID_ATTACH
};

class CAddLinkState
{
public:
    void setGraph(ptr<SRenderPassGraph> vGraph)
    {
        m_pGraph = vGraph;
        if (isStarted())
            end();
    }
    void start(const SRenderPassGraphPortInfo& vStartPort, bool vIsSource)
    {
        if (!m_pGraph)
            throw std::runtime_error("Graph is not set");
        if (!m_pGraph->hasPort(vStartPort.NodeId, vStartPort.Name, !vIsSource))
            throw std::runtime_error("Port not found in graph");
        m_FixedPort = vStartPort;
        m_IsFixedPortSource = vIsSource;
        m_IsAdding = true;
    }

    bool isStarted() const
    {
        return m_IsAdding;
    }

    void tick()
    {
        __assertStarted();
        m_AttachedPort.reset();
        m_AttachedPortPriority = -INFINITY;
        m_IsAttachedPortSource = false;
    }

    // TIPS: add any near port, this class will check if it's valid and give a result by getLinkState
    bool addCandidate(const SRenderPassGraphPortInfo& vTargetPort, bool vIsSource, float vPriority)
    {
        __assertStarted();
        if (!m_pGraph->hasPort(vTargetPort.NodeId, vTargetPort.Name, !vIsSource))
            throw std::runtime_error("Port not found in graph");

        // skip if it's the same node as fixed port
        if (vTargetPort.NodeId == m_FixedPort.NodeId) return false;

        // skip if already have this link
        SRenderPassGraphLink Link = __generateLink(vTargetPort);
        if (m_pGraph->hasLink(Link)) return false;

        // keep most important port
        if (!m_AttachedPort.has_value() || vPriority > m_AttachedPortPriority)
        {
            m_AttachedPort = vTargetPort;
            m_AttachedPortPriority = vPriority;
            m_IsAttachedPortSource = vIsSource;
        }
        return true;
    }

    const SRenderPassGraphPortInfo& getFixedPort() { __assertStarted(); return m_FixedPort; }
    bool isFixedPortSource() { __assertStarted(); return m_IsFixedPortSource; }

    EAddLinkAttachState getLinkState(std::string& voReason) const
    {
        if (!m_AttachedPort.has_value())
        {
            return EAddLinkAttachState::NOT_ATTACHED;
        }
        if (m_IsFixedPortSource == m_IsAttachedPortSource) // input-input or output-output
        {
            voReason = u8"不能连接同侧的Port";
            return EAddLinkAttachState::INVALID_ATTACH;
        }
        // TODO: loop check 
        return EAddLinkAttachState::VALID_ATTACH;
    }
    
    const SRenderPassGraphPortInfo& getCurrentAttachedPort()
    {
        if (!m_AttachedPort.has_value())
            throw std::runtime_error("No attached port for now, call getLinkState first to check if any port is attached");
        return m_AttachedPort.value();
    }

    SRenderPassGraphLink getCurrentValidLink()
    {
        std::string Message;
        if (getLinkState(Message) != EAddLinkAttachState::VALID_ATTACH)
            throw std::runtime_error("No valid attached link for now, call getLinkState first to check");

        return __generateLink(m_AttachedPort.value());
    }

    void end()
    {
        m_IsAdding = false;
    }

private:
    void __assertStarted()
    {
        if (!m_IsAdding)
            throw std::runtime_error("Not started");
    }

    SRenderPassGraphLink __generateLink(const SRenderPassGraphPortInfo& vAttachedPort)
    {
        __assertStarted();

        if (m_IsFixedPortSource)
            return SRenderPassGraphLink{ m_FixedPort , vAttachedPort };
        else
            return SRenderPassGraphLink{ vAttachedPort, m_FixedPort };
    }


    ptr<SRenderPassGraph> m_pGraph = nullptr;
    bool m_IsAdding = false;
    SRenderPassGraphPortInfo m_FixedPort = SRenderPassGraphPortInfo();
    bool m_IsFixedPortSource = true; // true: fixed source, find destination; false: fixed destination, find source
    std::optional<SRenderPassGraphPortInfo> m_AttachedPort;
    bool m_IsAttachedPortSource = false;
    float m_AttachedPortPriority = -INFINITY;
};

class CRenderPassGraphUI : public IDrawableUI
{
public:
    static void registerRenderPass(const std::string& vName, std::function<vk::IRenderPass::Ptr()> vCreateFunction);

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
    CAddLinkState m_AddLinkState;
    const float m_AttachMinDistance = 16.0f;
    CRenderPassGraphEditor m_Editor;
};