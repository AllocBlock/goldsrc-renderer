#pragma once
#include "RenderPassGraph.h"
#include "RenderPass.h"
#include "RenderPassGraphEditor.h"
#include "RenderPassGraphInstance.h"
#include "Timer.h"
#include "Maths.h"
#include "RenderPassGraphAddLinkState.h"

#include <string>
#include <glm/glm.hpp>

class CRenderPassGraphUI : public IDrawableUI
{
public:
    virtual void _renderUIV() override;

    void setContext(vk::CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo)
    {
        m_pDevice = vDevice;
        m_pAppInfo = vAppInfo;
    }

    void setGraph(ptr<SRenderPassGraph> vGraph)
    {
        m_pGraph = vGraph;
        m_Editor.setGraph(vGraph);
        m_AddLinkState.setGraph(vGraph);

        destroyPasses();
    }
    void update();
    void destroyPasses()
    {
        for (const auto& Pair : m_PassInstanceMap)
            Pair.second->destroy();
        m_PassInstanceMap.clear();
    }

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

    glm::vec2 __getPortPos(const SRenderPassGraphPortInfo& vPort, bool vIsInput) const
    {
        if (vIsInput)
            return m_NodePortPosMap.at(vPort.NodeId).Input.at(vPort.Name);
        else
            return m_NodePortPosMap.at(vPort.NodeId).Output.at(vPort.Name);
    }
    
    CPortSet::CPtr __getNodePortSet(size_t vNodeId)
    {
        const SRenderPassGraphNode& Node = m_pGraph->NodeMap.at(vNodeId);
        if (m_PassInstanceMap.find(vNodeId) == m_PassInstanceMap.end())
        {
            _ASSERTE(m_pDevice && m_pAppInfo);
            auto pPass = RenderpassLib::createPass(Node.Name);
            pPass->init(m_pDevice, m_pAppInfo);
            m_PassInstanceMap[vNodeId] = pPass;
        }
        auto pPass = m_PassInstanceMap.at(vNodeId);
        auto pPortSet = pPass->getPortSet();
        _ASSERTE(pPortSet);
        return pPortSet;
    }

    // TODO: cache to avoid redundant creation
    std::vector<std::string> __getNodeInputs(size_t vNodeId)
    {
        auto pPortSet = __getNodePortSet(vNodeId);

        std::vector<std::string> InputSet;
        for (size_t i = 0; i < pPortSet->getInputPortNum(); ++i)
        {
            InputSet.push_back(pPortSet->getInputPort(i)->getName());
        }
        return InputSet;
    }

    std::vector<std::string> __getNodeOutputs(size_t vNodeId)
    {
        auto pPortSet = __getNodePortSet(vNodeId);

        std::vector<std::string> OutputSet;
        for (size_t i = 0; i < pPortSet->getOutputPortNum(); ++i)
        {
            OutputSet.push_back(pPortSet->getOutputPort(i)->getName());
        }
        return OutputSet;
    }

    vk::CDevice::CPtr m_pDevice = nullptr;
    CAppInfo::Ptr m_pAppInfo = nullptr;
    ptr<SRenderPassGraph> m_pGraph = nullptr;
    std::map<size_t, vk::IRenderPass::Ptr> m_PassInstanceMap;

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
    CRenderGraphAddLinkState m_AddLinkState;
    CRenderPassGraphEditor m_Editor;
};