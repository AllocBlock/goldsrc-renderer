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

class CLinkEvent
{
public:
    size_t hookImageUpdate(std::function<void()> vCallback)
    {
        m_ImageHookMap[m_CurImageHookId] = vCallback;
        return m_CurImageHookId++;
    }

    void unhookImageUpdate(size_t vHookId)
    {
        if (m_ImageHookMap.find(vHookId) != m_ImageHookMap.end())
            m_ImageHookMap.erase(vHookId);
    }

    size_t hookLinkUpdate(std::function<void()> vCallback)
    {
        m_LinkHookMap[m_CurLinkHookId] = vCallback;
        return m_CurLinkHookId++;
    }

    void unhookLinkUpdate(size_t vHookId)
    {
        if (m_LinkHookMap.find(vHookId) != m_LinkHookMap.end())
            m_LinkHookMap.erase(vHookId);
    }
protected:
    void _onImageUpdate()
    {
        for (const auto& Pair : m_ImageHookMap)
            Pair.second();

        _onImageUpdateExtendV();
    }

    void _onLinkUpdate()
    {
        for (const auto& Pair : m_LinkHookMap)
            Pair.second();

        _onLinkUpdateExtendV();
    }

    virtual void _onImageUpdateExtendV() {}
    virtual void _onLinkUpdateExtendV() {}

private:

    size_t m_CurImageHookId = 1;
    size_t m_CurLinkHookId = 1;
    std::map<size_t, std::function<void()>> m_ImageHookMap;
    std::map<size_t, std::function<void()>> m_LinkHookMap;
};

class CPort : public CLinkEvent, public std::enable_shared_from_this<CPort>
{
public:
    _DEFINE_PTR(CPort);

    CPort() = delete;
    CPort(const SPortFormat& vFormat): m_Format(vFormat) {}

    // FIXME: better solution?
    bool isSwapchainSource() { return m_isSwapchainSource; }
    void markAsSwapchainSource() { m_isSwapchainSource = true; }

    bool isRoot() { return m_pParent.expired(); }
    bool isTail() { return !hasChildren(); }
    CPort::Ptr getRoot()
    {
        CPort::Ptr pCur = shared_from_this();
        while (!pCur->isRoot()) pCur = getParent();
        return pCur;
    }
    CPort::Ptr getParent() { return m_pParent.expired() ? nullptr : m_pParent.lock(); }
    bool hasParent() { return !m_pParent.expired(); }
    bool hasChildren() { return !m_ChildSet.empty(); }
    size_t getChildNum() { return m_ChildSet.size(); }
    CPort::Ptr getChild(size_t vIndex) { return m_ChildSet[vIndex]; }

    void append(CPort::Ptr vPort)
    {
        vPort->attachTo(shared_from_this());
    }
    void clearChildren() { m_ChildSet.clear(); }

    void attachTo(CPort::Ptr vPort)
    {
        _ASSERTE(isMatch(vPort));

        m_pParent = vPort;
        vPort->m_ChildSet.emplace_back(shared_from_this());

        _onLinkUpdate();
        vPort->_onLinkUpdate();
    }

    virtual bool isReadyV() const = 0; // if the link of this port is ready, which means it has a source port as head

    const SPortFormat& getFormat() const { return m_Format; }
    virtual bool hasActualFormatV() const = 0;
    virtual const SPortFormat& getActualFormatV() const = 0;
    virtual VkImageView getImageV(size_t vIndex = 0) const = 0;

    bool isMatch(CPort::CPtr vPort)
    {
        return m_Format.isMatch(vPort->getFormat());
    }

protected:
    virtual void _onImageUpdateExtendV() override final
    {
        for (const auto& pChild : m_ChildSet)
            pChild->_onImageUpdate();
    }

    bool m_isSwapchainSource = false;

    SPortFormat m_Format;

    wptr<CPort> m_pParent;
    std::vector<CPort::Ptr> m_ChildSet;
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

    virtual bool hasActualFormatV() const override final
    {
        return m_IsActualFormatSet;
    }

    virtual const SPortFormat& getActualFormatV() const override final
    {
        _ASSERTE(m_IsActualFormatSet);
        return { m_ActualFormat, m_ActualExtent, m_ImageMap.size()};
    }

    virtual bool isReadyV() const override final
    {
        return true;
    }

    void setImage(VkImageView vImage, size_t vIndex = 0)
    {
        if (m_ImageMap.find(vIndex) == m_ImageMap.end() || m_ImageMap[vIndex] != vImage)
        {
            m_ImageMap[vIndex] = vImage;
            _onImageUpdate();
        }
    }

    void setActualFormat(VkFormat vFormat, VkExtent2D vExtent)
    {
        m_IsActualFormatSet = true;
        m_ActualFormat = vFormat;
        m_ActualExtent = vExtent;
    }

private:
    std::map<size_t, VkImageView> m_ImageMap;

    bool m_IsActualFormatSet = false;
    VkFormat m_ActualFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkExtent2D m_ActualExtent = VkExtent2D{ 0, 0 };
};

class CRelayPort : public CPort
{
public:
    _DEFINE_PTR(CRelayPort);

    CRelayPort(const SPortFormat& vFormat) : CPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        _ASSERTE(!m_pParent.expired());
        return m_pParent.lock()->getImageV(vIndex);
    }

    virtual bool hasActualFormatV() const override final
    {
        if (m_pParent.expired()) return false;
        else return m_pParent.lock()->hasActualFormatV();
    }

    virtual const SPortFormat& getActualFormatV() const override final
    {
        _ASSERTE(!m_pParent.expired());
        return m_pParent.lock()->getActualFormatV();
    }

    virtual bool isReadyV() const override final
    {
        if (m_pParent.expired()) return false;
        else return m_pParent.lock()->isReadyV();
    }
};

class SPortDescriptor
{
public:
    void addInput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyFormat)
    {
        _ASSERTE(!hasInput(vName));
        _ASSERTE(!hasInputOutput(vName));

        m_InputPortNameSet.emplace_back(vName);
        m_InputPortSet.emplace_back(vFormat);
    }

    void addOutput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyFormat)
    {
        _ASSERTE(!hasOutput(vName));
        _ASSERTE(!hasInputOutput(vName));

        m_OutputPortNameSet.emplace_back(vName);
        m_OutputPortSet.emplace_back(vFormat);
    }

    void addInputOutput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyFormat)
    {
        _ASSERTE(!hasInput(vName));
        _ASSERTE(!hasOutput(vName));
        _ASSERTE(!hasInputOutput(vName));

        m_InputOutputPortNameSet.emplace_back(vName);
        m_InputOutputPortSet.emplace_back(vFormat);
    }

    void clear()
    {
        m_InputPortNameSet.clear();
        m_InputPortSet.clear();
        m_OutputPortNameSet.clear();
        m_OutputPortSet.clear();
        m_InputOutputPortNameSet.clear();
        m_InputOutputPortSet.clear();
    }

    bool hasInput(const std::string vName) const 
    { 
        return std::find(m_InputPortNameSet.begin(), m_InputPortNameSet.end(), vName) != m_InputPortNameSet.end();
    }
    bool hasOutput(const std::string vName) const
    {
        return std::find(m_OutputPortNameSet.begin(), m_OutputPortNameSet.end(), vName) != m_OutputPortNameSet.end();
    }
    bool hasInputOutput(const std::string vName) const
    {
        return std::find(m_InputOutputPortNameSet.begin(), m_InputOutputPortNameSet.end(), vName) != m_InputOutputPortNameSet.end();
    }

    std::vector<std::string> m_InputPortNameSet;
    std::vector<SPortFormat> m_InputPortSet;
    std::vector<std::string> m_OutputPortNameSet;
    std::vector<SPortFormat> m_OutputPortSet;
    std::vector<std::string> m_InputOutputPortNameSet;
    std::vector<SPortFormat> m_InputOutputPortSet;
};

class CPortSet : public CLinkEvent
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

        for (size_t i = 0; i < vDesc.m_InputOutputPortNameSet.size(); ++i)
        {
            __addInputOutput(vDesc.m_InputOutputPortNameSet[i], vDesc.m_InputOutputPortSet[i]);
        }
    }

    bool isReady()
    {
        for (const auto& Pair : m_InputPortMap)
        {
            if (!Pair.second->isReadyV()) return false;
        }
        for (const auto& Pair : m_OutputPortMap)
        {
            if (!Pair.second->isReadyV()) return false;
        }
        return true;
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

    void setOutput(const std::string& vOutputName, VkImageView vImage, VkFormat vFormat, VkExtent2D vExtent, size_t vIndex = 0)
    {
        SPortFormat Format;
        Format.Format = vFormat;
        Format.Extent = vExtent;
        Format.Num = 0; // dont care

        auto pPort = getOutputPort(vOutputName);
        _ASSERTE(Format.isMatch(pPort->getFormat()));

        CSourcePort::Ptr pSourcePort = std::dynamic_pointer_cast<CSourcePort>(pPort);

        // FIXME: what if different formats/extents are set? how to deal this?
        pSourcePort->setActualFormat(vFormat, vExtent);
        pSourcePort->setImage(vImage, vIndex);
    }

    void setOutput(const std::string& vOutputName, vk::CImage::CPtr vImage, size_t vIndex = 0)
    {
        setOutput(vOutputName, *vImage, vImage->getFormat(), { vImage->getWidth(), vImage->getHeight() }, vIndex);
    }

    void append(const std::string& vInputName, CPort::Ptr vPort)
    {
        auto pPort = getInputPort(vInputName);
        pPort->append(vPort);
    }

    void attachTo(const std::string& vInputName, CPort::Ptr vPort)
    {
        auto pPort = getInputPort(vInputName);
        pPort->attachTo(vPort);
    }

    static void link(CPort::Ptr vOutputPort, CPort::Ptr vInputPort)
    {
        _ASSERTE(vOutputPort);
        _ASSERTE(vInputPort);
        vOutputPort->append(vInputPort);
    }

    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPort::Ptr vInputPort)
    {
        link(vSet1->getOutputPort(vOutputName), vInputPort);
    }

    static void link(CPort::Ptr vOutputPort, CPortSet::Ptr vSet2, const std::string& vInputName)
    {
        link(vOutputPort, vSet2->getInputPort(vInputName));
    }

    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPortSet::Ptr vSet2, const std::string& vInputName)
    {
        link(vSet1->getOutputPort(vOutputName), vSet2->getInputPort(vInputName));
    }

private:
    void __addInput(const std::string& vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasInput(vName));
        m_InputPortMap[vName] = make<CRelayPort>(vFormat);
        m_InputPortMap[vName]->hookImageUpdate(m_pImageUpdateCallbackFunc);
        m_InputPortMap[vName]->hookLinkUpdate(m_pLinkUpdateCallbackFunc);
    }

    void __addOutput(const std::string& vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasOutput(vName));
        m_OutputPortMap[vName] = make<CSourcePort>(vFormat);
        m_OutputPortMap[vName]->hookImageUpdate(m_pImageUpdateCallbackFunc);
        m_OutputPortMap[vName]->hookLinkUpdate(m_pLinkUpdateCallbackFunc);
    }

    void __addInputOutput(const std::string& vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasOutput(vName));
        _ASSERTE(!hasInput(vName));
        CRelayPort::Ptr pPort = make<CRelayPort>(SPortFormat::AnyFormat);
        pPort->hookImageUpdate(m_pImageUpdateCallbackFunc);
        pPort->hookLinkUpdate(m_pLinkUpdateCallbackFunc);
        m_InputPortMap[vName] = m_OutputPortMap[vName] = pPort;
    }

    std::function<void()> m_pImageUpdateCallbackFunc = [this]() { _onImageUpdate(); };
    std::function<void()> m_pLinkUpdateCallbackFunc = [this]() { _onLinkUpdate(); };
    std::map<std::string, CPort::Ptr> m_InputPortMap;
    std::map<std::string, CPort::Ptr> m_OutputPortMap;
};