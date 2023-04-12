#pragma once
#include "Pointer.h"
#include "RenderPassGraph.h"

#include <string>

enum class EAddLinkAttachState
{
    NOT_ATTACHED,
    INVALID_ATTACH,
    VALID_ATTACH
};

class CRenderGraphAddLinkState
{
public:
    void setGraph(ptr<SRenderPassGraph> vGraph);
    void start(const SRenderPassGraphPortInfo& vStartPort, bool vIsSource);
    void clearCandidates();
    // TIPS: add any near port, this class will check if it's valid and give a result by getLinkState
    bool addCandidate(const SRenderPassGraphPortInfo& vTargetPort, bool vIsSource, float vPriority);

    bool isStarted() const;
    const SRenderPassGraphPortInfo& getFixedPort();
    bool isFixedPortSource();
    EAddLinkAttachState getLinkState(std::string& voReason) const;
    const SRenderPassGraphPortInfo& getCurrentAttachedPort();
    SRenderPassGraphLink getCurrentValidLink();

    void end();

private:
    void __assertStarted();

    SRenderPassGraphLink __generateLink(const SRenderPassGraphPortInfo& vAttachedPort);


    ptr<SRenderPassGraph> m_pGraph = nullptr;
    bool m_IsAdding = false;
    SRenderPassGraphPortInfo m_FixedPort = SRenderPassGraphPortInfo();
    bool m_IsFixedPortSource = true; // true: fixed source, find destination; false: fixed destination, find source
    std::optional<SRenderPassGraphPortInfo> m_AttachedPort;
    bool m_IsAttachedPortSource = false;
    float m_AttachedPortPriority = -INFINITY;
};