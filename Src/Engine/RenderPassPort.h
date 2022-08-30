#pragma once
#include "PchVulkan.h"
#include "Image.h"

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <set>

struct SPortFormat
{
    VkFormat Format = VkFormat::VK_FORMAT_UNDEFINED; // VK_FORMAT_UNDEFINED for any
    VkExtent2D Extent = { 0, 0 }; // (0, 0) for any extent(>0)
    size_t Num = 1; // 0 for any num (>0)

    bool isMatch(const SPortFormat& v) const
    {
        return (Format == v.Format || Format == VkFormat::VK_FORMAT_UNDEFINED || v.Format == VkFormat::VK_FORMAT_UNDEFINED)
            && (Extent.width == v.Extent.width || Extent.width == 0 || v.Extent.width == 0)
            && (Extent.height == v.Extent.height || Extent.height == 0 || v.Extent.height == 0)
            && (Num == v.Num || Num == 0 || v.Num == 0);
    }

    static const SPortFormat& AnyFormat;
};

class CPort : public std::enable_shared_from_this<CPort>
{
public:
    _DEFINE_PTR(CPort);

    CPort() = delete;
    CPort(const SPortFormat& vFormat): m_Format(vFormat) {}

    bool isHead() { return m_Prev.expired(); }
    CPort::Ptr getHead()
    {
        wptr<CPort> pCur = weak_from_this();
        while (!pCur.lock()->isHead()) pCur = getPrev();
        return pCur.lock();
    }
    CPort::Ptr getPrev() { return m_Prev.expired() ? nullptr : m_Prev.lock(); }
    CPort::Ptr getNext() { return m_Next.expired() ? nullptr : m_Next.lock(); }

    void linkTo(CPort::Ptr vPort) { linkAfter(vPort); }

    void linkAfter(CPort::Ptr vPort)
    {
        vPort->linkBefore(shared_from_this());
    }

    void linkBefore(CPort::Ptr vPort)
    {
        _ASSERTE(isMatch(vPort));

        m_Prev = vPort;
        vPort->m_Next = shared_from_this();
    }

    const SPortFormat& getFormat() const { return m_Format; }
    virtual VkImageView getImageV(size_t vIndex = 0) const = 0;

    bool isMatch(CPort::CPtr vPort)
    {
        return m_Format.isMatch(vPort->getFormat());
    }

    size_t hookUpdate(std::function<void()> vCallback)
    {
        m_HookMap[m_CurHookId] = vCallback;
        return m_CurHookId++;
    }

    void unhookUpdate(size_t vHookId)
    {
        if (m_HookMap.find(vHookId) != m_HookMap.end())
            m_HookMap.at(vHookId);
    }

protected:
    void _triggerUpdate()
    {
        if (!m_Next.expired())
            m_Next.lock()->_triggerUpdate();

        for (const auto& Pair : m_HookMap)
            Pair.second();
    }

    SPortFormat m_Format;

    wptr<CPort> m_Prev;
    wptr<CPort> m_Next;

    size_t m_CurHookId = 1;
    std::map<size_t, std::function<void()>> m_HookMap;
};

class CSourcePort : public CPort
{
public:
    _DEFINE_PTR(CSourcePort);

    CSourcePort(const SPortFormat& vFormat) : CPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        _ASSERTE(m_ImageMap.find(vIndex) != m_ImageMap.end());
        return m_ImageMap.at(vIndex);
    }

    void setImage(VkImageView vImage, size_t vIndex = 0)
    {
        if (m_ImageMap.find(vIndex) == m_ImageMap.end() || m_ImageMap[vIndex] != vImage)
        {
            m_ImageMap[vIndex] = vImage;
            _triggerUpdate();
        }
    }

private:
    std::map<size_t, VkImageView> m_ImageMap;
};

class CRelayPort : public CPort
{
public:
    _DEFINE_PTR(CRelayPort);

    CRelayPort(const SPortFormat& vFormat) : CPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        _ASSERTE(!m_Prev.expired());
        return m_Prev.lock()->getImageV(vIndex);
    }
};

class SPortDescriptor
{
public:
    void addInput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyFormat)
    {
        _ASSERTE(!hasInput(vName));
        m_InputPortNameSet.emplace_back(vName);
        m_InputPortSet.emplace_back(vFormat);
    }

    void addOutput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyFormat)
    {
        _ASSERTE(!hasOutput(vName));
        _ASSERTE(!hasInputSrcOutput(vName));
        m_OutputPortNameSet.emplace_back(vName);
        m_OutputPortSet.emplace_back(vFormat);
    }

    void addInputSrcOutput(std::string vName, std::string vInputName)
    {

        _ASSERTE(!hasOutput(vName));
        _ASSERTE(!hasInputSrcOutput(vName));
        _ASSERTE(hasInput(vInputName));
        m_InputSrcOutputPortNameSet.emplace_back(vName);
        m_InputSrcOutputPortTargetSet.emplace_back(vInputName);
    }

    void clear()
    {
        m_InputPortNameSet.clear();
        m_InputPortSet.clear();
        m_OutputPortNameSet.clear();
        m_OutputPortSet.clear();
        m_InputSrcOutputPortNameSet.clear();
        m_InputSrcOutputPortTargetSet.clear();
    }

    bool hasInput(const std::string vName) const 
    { 
        return std::find(m_InputPortNameSet.begin(), m_InputPortNameSet.end(), vName) != m_InputPortNameSet.end();
    }
    bool hasOutput(const std::string vName) const
    {
        return std::find(m_OutputPortNameSet.begin(), m_OutputPortNameSet.end(), vName) != m_OutputPortNameSet.end();
    }
    bool hasInputSrcOutput(const std::string vName) const
    {
        return std::find(m_InputSrcOutputPortNameSet.begin(), m_InputSrcOutputPortNameSet.end(), vName) != m_InputSrcOutputPortNameSet.end();
    }

    std::vector<std::string> m_InputPortNameSet;
    std::vector<SPortFormat> m_InputPortSet;
    std::vector<std::string> m_OutputPortNameSet;
    std::vector<SPortFormat> m_OutputPortSet;
    std::vector<std::string> m_InputSrcOutputPortNameSet;
    std::vector<std::string> m_InputSrcOutputPortTargetSet;
};

class CPortSet
{
public:
    _DEFINE_PTR(CPortSet);

    CPortSet() = delete;
    CPortSet(const SPortDescriptor& vDesc)
    {
        for (size_t i = 0; i < vDesc.m_InputPortNameSet.size(); ++i)
        {
            __addInput(vDesc.m_InputPortNameSet[i], vDesc.m_InputPortSet[i]);
        }
        for (size_t i = 0; i < vDesc.m_OutputPortNameSet.size(); ++i)
        {
            __addOutput(vDesc.m_OutputPortNameSet[i], vDesc.m_OutputPortSet[i]);
        }

        for (size_t i = 0; i < vDesc.m_InputSrcOutputPortNameSet.size(); ++i)
        {
            __addInputSrcOutput(vDesc.m_InputSrcOutputPortNameSet[i], vDesc.m_InputSrcOutputPortTargetSet[i]);
        }
    }

    bool hasInput(const std::string& vName) const { return m_InputPortMap.find(vName) != m_InputPortMap.end(); }
    bool hasOutput(const std::string& vName) const { return m_OutputPortMap.find(vName) != m_OutputPortMap.end(); }

    CPort::Ptr getInputPort(const std::string& vName)
    {
        _ASSERTE(hasInput(vName));
        return m_InputPortMap.at(vName);
    }

    CPort::Ptr getOutputPort(const std::string& vName)
    {
        _ASSERTE(hasOutput(vName));
        return m_OutputPortMap.at(vName);
    }

    const SPortFormat& getInputFormat(const std::string& vName)
    {
        _ASSERTE(hasInput(vName));
        return getInputPort(vName)->getFormat();
    }

    const SPortFormat& getOutputFormat(const std::string& vName)
    {
        _ASSERTE(hasOutput(vName));
        return getOutputPort(vName)->getFormat();
    }

    void setOutput(const std::string& vOutputName, VkImageView vImage, const SPortFormat& vFormat, size_t vIndex = 0)
    {
        // TODO: how to store actual format in port? or is this important?
        auto pPort = getOutputPort(vOutputName);
        _ASSERTE(vFormat.isMatch(pPort->getFormat()));

        CSourcePort::Ptr pSourcePort = std::dynamic_pointer_cast<CSourcePort>(pPort);
        pSourcePort->setImage(vImage, vIndex);
    }

    void setOutput(const std::string& vOutputName, vk::CImage::CPtr vImage, size_t vIndex = 0)
    {
        SPortFormat Format;
        Format.Format = vImage->getFormat();
        Format.Extent.width = vImage->getWidth();
        Format.Extent.height = vImage->getHeight();
        Format.Num = 0; // dont care

        setOutput(vOutputName, *vImage, Format, vIndex);
    }

    void linkTo(const std::string& vInputName, CPort::Ptr vPort)
    {
        auto pPort = getInputPort(vInputName);
        pPort->linkTo(vPort);
    }

    void linkFrom(const std::string& vInputName, CPort::Ptr vPort)
    {
        auto pPort = getInputPort(vInputName);
        pPort->linkBefore(vPort);
    }

    static void link(CPort::Ptr vOutputPort, CPort::Ptr vInputPort)
    {
        vOutputPort->linkTo(vInputPort);
    }

    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPort::Ptr vInputPort)
    {
        vSet1->getOutputPort(vOutputName)->linkTo(vInputPort);
    }

    static void link(CPort::Ptr vOutputPort, CPortSet::Ptr vSet2, const std::string& vInputName)
    {
        vOutputPort->linkTo(vSet2->getInputPort(vInputName));
    }

    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPortSet::Ptr vSet2, const std::string& vInputName)
    {
        vSet1->getOutputPort(vOutputName)->linkTo(vSet2->getInputPort(vInputName));
    }

private:
    void __addInput(const std::string& vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasInput(vName));
        m_InputPortMap[vName] = make<CRelayPort>(vFormat);
    }

    void __addOutput(const std::string& vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasOutput(vName));
        m_OutputPortMap[vName] = make<CSourcePort>(vFormat);
    }

    void __addInputSrcOutput(const std::string& vName, const std::string& vInputName)
    {
        _ASSERTE(!hasOutput(vName));
        _ASSERTE(hasInput(vInputName));
        CRelayPort::Ptr pPort = make<CRelayPort>(SPortFormat::AnyFormat);
        link(m_InputPortMap.at(vInputName), pPort);
        m_OutputPortMap[vName] = pPort;
    }

    std::map<std::string, CPort::Ptr> m_InputPortMap;
    std::map<std::string, CPort::Ptr> m_OutputPortMap;
};