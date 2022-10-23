#pragma once
#include "PchVulkan.h"
#include "Image.h"

#include <string>
#include <vector>
#include <map>
#include <functional>

enum class EUsage
{
    DONT_CARE,
    UNDEFINED,
    READ, // for sampling
    WRITE, // as render target
    PRESENTATION
};

VkImageLayout toLayout(EUsage vUsage, bool isDepth = false);

struct SPortFormat
{
    VkFormat Format = VkFormat::VK_FORMAT_UNDEFINED; // VK_FORMAT_UNDEFINED for any
    VkExtent2D Extent = { 0, 0 }; // (0, 0) for any extent(>0)
    size_t Num = 1; // 0 for any num (>0)

    EUsage Usage = EUsage::DONT_CARE;

    bool isMatch(const SPortFormat& v) const;

    static SPortFormat createAnyOfUsage(EUsage vUsage);

    static const SPortFormat& AnyPortFormat;
    static const VkFormat& AnyFormat;
    static const VkExtent2D& AnyExtent;
    static const size_t AnyNum;
};

class ILinkEvent
{
public:
    _DEFINE_PTR(ILinkEvent);

    using EventId_t = std::string;
    static EventId_t generateEventId(std::string vPrefix);

    size_t hookImageUpdate(std::function<void()> vCallback);
    void unhookImageUpdate(size_t vHookId);
    size_t hookLinkUpdate(std::function<void(EventId_t, ILinkEvent::CPtr)> vCallback);
    void unhookLinkUpdate(size_t vHookId);

protected:
    void _onImageUpdate();
    void _onLinkUpdate(EventId_t vEventId, ILinkEvent::CPtr vFrom);

    virtual void _onImageUpdateExtendV() {}
    virtual void _onLinkUpdateExtendV(EventId_t vEventId, ILinkEvent::CPtr vFrom) {}

private:
    static size_t CurEventId;
    size_t m_CurImageHookId = 1;
    size_t m_CurLinkHookId = 1;
    std::map<size_t, std::function<void()>> m_ImageHookMap;
    std::map<size_t, std::function<void(EventId_t, ILinkEvent::CPtr)>> m_LinkHookMap;
};

class CPort : public ILinkEvent, public std::enable_shared_from_this<CPort>
{
public:
    _DEFINE_PTR(CPort);

    CPort() = delete;
    CPort(const SPortFormat& vFormat): m_Format(vFormat) {}

    virtual ~CPort() = default;
    
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

    void attachTo(CPort::Ptr vPort);

    // if the link of this port is ready, means it has a source port as head and no in valid port before it
    virtual bool isReadyV() const
    {
        return !m_ForceNotReady;
    }

    void setForceNotReady(bool vForceNotReady);
    bool isForceNotReady() const { return m_ForceNotReady; }

    const SPortFormat& getFormat() const { return m_Format; }
    virtual VkImageView getImageV(size_t vIndex = 0) const = 0;

    bool isMatch(CPort::CPtr vPort)
    {
        return m_Format.isMatch(vPort->getFormat());
    }
    
    virtual bool hasActualFormatV() const = 0;
    virtual VkFormat getActualFormatV() const = 0;
    virtual bool hasActualExtentV() const = 0;
    virtual VkExtent2D getActualExtentV() const = 0;

protected:
    // image update propagates to tail
    virtual void _onImageUpdateExtendV() override final;

    // link update propagates to whole link
    virtual void _onLinkUpdateExtendV(EventId_t vEventId, ILinkEvent::CPtr vFrom) override final;

    bool m_isSwapchainSource = false;

    SPortFormat m_Format;

    wptr<CPort> m_pParent;
    std::vector<CPort::Ptr> m_ChildSet;

private:
    bool m_ForceNotReady = false;
};

class CSourcePort : public CPort
{
public:
    _DEFINE_PTR(CSourcePort);

    CSourcePort(const SPortFormat& vFormat);

    virtual VkImageView getImageV(size_t vIndex = 0) const override final;
    virtual bool isReadyV() const override final;
    void setImage(VkImageView vImage, size_t vIndex = 0);
    
    virtual bool hasActualFormatV() const override final { return m_ActualFormat != VK_FORMAT_UNDEFINED; }
    virtual bool hasActualExtentV() const override final { return m_ActualExtent.width != 0 && m_ActualExtent.height != 0; }
    virtual VkFormat getActualFormatV() const override final { _ASSERTE(hasActualFormatV()); return m_ActualFormat; }
    virtual VkExtent2D getActualExtentV() const override final { _ASSERTE(hasActualExtentV()); return m_ActualExtent; }
    void setActualFormat(VkFormat vFormat) { m_ActualFormat = vFormat; }
    void setActualExtent(VkExtent2D vExtent) { m_ActualExtent = vExtent; }

private:
    std::map<size_t, VkImageView> m_ImageMap;
    
    VkFormat m_ActualFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkExtent2D m_ActualExtent = VkExtent2D{ 0, 0 };
};

class CRelayPort : public CPort
{
public:
    _DEFINE_PTR(CRelayPort);

    CRelayPort(const SPortFormat& vFormat) : CPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override final;
    virtual bool isReadyV() const override final;

    virtual bool hasActualFormatV() const override final { return !m_pParent.expired() && m_pParent.lock()->hasActualFormatV(); }
    virtual bool hasActualExtentV() const override final { return !m_pParent.expired() && m_pParent.lock()->hasActualExtentV(); }
    virtual VkFormat getActualFormatV() const override final { _ASSERTE(hasActualFormatV()); return m_pParent.lock()->getActualFormatV(); }
    virtual VkExtent2D getActualExtentV() const override final { _ASSERTE(hasActualExtentV()); return m_pParent.lock()->getActualExtentV();
    }
};

class SPortDescriptor
{
public:
    void addInput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyPortFormat);
    void addOutput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyPortFormat);
    void addInputOutput(std::string vName, const SPortFormat& vFormat = SPortFormat::AnyPortFormat);
    void clear();
    bool hasInput(const std::string vName) const;
    bool hasOutput(const std::string vName) const;
    bool hasInputOutput(const std::string vName) const;

    std::vector<std::string> m_InputPortNameSet;
    std::vector<SPortFormat> m_InputPortSet;
    std::vector<std::string> m_OutputPortNameSet;
    std::vector<SPortFormat> m_OutputPortSet;
    std::vector<std::string> m_InputOutputPortNameSet;
    std::vector<SPortFormat> m_InputOutputPortSet;
};

class CPortSet : public ILinkEvent
{
public:
    _DEFINE_PTR(CPortSet);

    CPortSet() = delete;
    CPortSet(const SPortDescriptor& vDesc);

    bool isReady();

    bool hasInput(const std::string& vName) const;
    bool hasOutput(const std::string& vName) const;

    CPort::Ptr getInputPort(const std::string& vName);
    CPort::Ptr getOutputPort(const std::string& vName);
    const SPortFormat& getInputFormat(const std::string& vName);
    const SPortFormat& getOutputFormat(const std::string& vName);

    void setOutput(const std::string& vOutputName, VkImageView vImage, VkFormat vFormat, VkExtent2D vExtent, size_t vIndex = 0);
    void setOutput(const std::string& vOutputName, const vk::CImage& vImage, size_t vIndex = 0);
    void append(const std::string& vInputName, CPort::Ptr vPort);
    void attachTo(const std::string& vInputName, CPort::Ptr vPort);

    static void link(CPort::Ptr vOutputPort, CPort::Ptr vInputPort);
    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPort::Ptr vInputPort);
    static void link(CPort::Ptr vOutputPort, CPortSet::Ptr vSet2, const std::string& vInputName);
    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPortSet::Ptr vSet2, const std::string& vInputName);

private:
    void __addInput(const std::string& vName, const SPortFormat& vFormat);
    void __addOutput(const std::string& vName, const SPortFormat& vFormat);
    void __addInputOutput(const std::string& vName, const SPortFormat& vFormat);

    std::function<void()> m_pImageUpdateCallbackFunc = nullptr;
    std::function<void(EventId_t, ILinkEvent::CPtr)> m_pLinkUpdateCallbackFunc = nullptr;
    std::map<std::string, CPort::Ptr> m_InputPortMap;
    std::map<std::string, CPort::Ptr> m_OutputPortMap;

    std::string m_LastEventId = "";
};