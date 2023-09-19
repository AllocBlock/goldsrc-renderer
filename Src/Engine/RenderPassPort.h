#pragma once
#include "PchVulkan.h"
#include "Image.h"
#include "Common.h"

#include <string>
#include <vector>
#include <map>

class CPortSet;

enum class EImageUsage
{
    UNKNOWN,
    READ, // for sampling
    COLOR_ATTACHMENT, // as color render target
    DEPTH_ATTACHMENT, // as depth buffer
    PRESENTATION, // as depth buffer
};

struct SPortInfo
{
    VkFormat Format = VkFormat::VK_FORMAT_UNDEFINED; // VK_FORMAT_UNDEFINED for any
    VkExtent2D Extent = { 0, 0 }; // (0, 0) for any extent(>0)
    size_t Num = 1; // 0 for any num (>0)
    EImageUsage Usage = EImageUsage::UNKNOWN;

    bool isMatch(const SPortInfo& v) const;

    static SPortInfo createAnyOfUsage(EImageUsage vUsage);

    static const SPortInfo& AnyPortInfo;
    static const VkFormat& AnyFormat;
    static const VkExtent2D& AnyExtent;
    static const size_t AnyNum;
};

class CPort : public std::enable_shared_from_this<CPort>
{
public:
    _DEFINE_PTR(CPort);

    CPort() = delete;
    CPort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet);
    virtual ~CPort() = default;
    
    _DEFINE_GETTER(Name, std::string)
    void setName(const std::string& vName) { _ASSERTE(!vName.empty()); m_Name = vName; }

    bool isRoot() const { return !hasParent(); }
    bool isTail() const { return !hasChildren(); }
    bool hasParent() const { return !m_pParent.expired(); }
    CPort::Ptr getParent() const { return m_pParent.expired() ? nullptr : m_pParent.lock(); }
    void removeParent();
    bool hasChildren() const { return !m_ChildSet.empty(); }
    size_t getChildNum() const { return m_ChildSet.size(); }
    CPort::Ptr getChild(size_t vIndex) const { return m_ChildSet[vIndex]; }
    void append(CPort::Ptr vPort) { vPort->attachTo(shared_from_this()); }
    void clearAllChildren();
    void attachTo(CPort::Ptr vPort);
    void unlinkAll();
    
    bool hasInputLayout() const;
    VkImageLayout getInputLayout() const;
    void setInputLayout(VkImageLayout vLayout);
    bool hasOutputLayout() const;
    VkImageLayout getOutputLayout() const;
    void setOutputLayout(VkImageLayout vLayout);

    bool isReady() const;
    void assertReady() const;

    const SPortInfo& getInfo() const { return m_Info; }

    virtual VkImageView getImageV(size_t vIndex = 0) const = 0;
    virtual size_t getImageNumV() const = 0;
    virtual bool hasActualFormatV() const = 0;
    virtual VkFormat getActualFormatV() const = 0;
    virtual bool hasActualExtentV() const = 0;
    virtual VkExtent2D getActualExtentV() const = 0;
    virtual bool hasActualNumV() const = 0;
    virtual size_t getActualNumV() const = 0;

    virtual bool isStandaloneSourceV() const = 0;

    bool isMatch(CPort::CPtr vPort) const { return m_Info.isMatch(vPort->getInfo()); }
    
    CPortSet* getBelongedPortSet() const { return m_pBelongedSet; }

protected:
    std::string m_Name = "";
    CPortSet* const m_pBelongedSet;
    wptr<CPort> m_pParent;
    std::vector<CPort::Ptr> m_ChildSet;
    
    SPortInfo m_Info;
    std::optional<VkImageLayout> m_InputLayout = std::nullopt;
    std::optional<VkImageLayout> m_OutputLayout = std::nullopt;
};

class CSourcePort : public CPort
{
public:
    _DEFINE_PTR(CSourcePort);

    CSourcePort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet);

    virtual VkImageView getImageV(size_t vIndex = 0) const override final;
    virtual size_t getImageNumV() const override final;
    void setImage(VkImageView vImage, size_t vIndex = 0);
    void clearImage() { m_ImageMap.clear(); }
    
    virtual bool hasActualFormatV() const override final { return m_ActualFormat != VK_FORMAT_UNDEFINED; }
    virtual VkFormat getActualFormatV() const override final { _ASSERTE(hasActualFormatV()); return m_ActualFormat; }
    virtual bool hasActualExtentV() const override final { return m_ActualExtent.width != 0 && m_ActualExtent.height != 0; }
    virtual VkExtent2D getActualExtentV() const override final { _ASSERTE(hasActualExtentV()); return m_ActualExtent; }
    virtual bool hasActualNumV() const override final { return m_ActualNum != SPortInfo::AnyNum; }
    virtual size_t getActualNumV() const override final { return m_ActualNum; }
    void setActualFormat(VkFormat vFormat) { m_ActualFormat = vFormat; }
    void setActualExtent(VkExtent2D vExtent) { m_ActualExtent = vExtent; }
    void setActualNum(size_t vImageNum) { m_ActualNum = vImageNum; }
    
    virtual bool isStandaloneSourceV() const override { return m_pBelongedSet == nullptr; }

private:
    std::map<size_t, VkImageView> m_ImageMap;
    
    VkFormat m_ActualFormat = SPortInfo::AnyFormat;
    VkExtent2D m_ActualExtent = SPortInfo::AnyExtent;
    size_t m_ActualNum = SPortInfo::AnyNum;
};

class CRelayPort : public CPort
{
public:
    _DEFINE_PTR(CRelayPort);

    CRelayPort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet);

    virtual VkImageView getImageV(size_t vIndex = 0) const override final;
    virtual size_t getImageNumV() const override final;

    virtual bool hasActualFormatV() const override final { return !m_pParent.expired() && m_pParent.lock()->hasActualFormatV(); }
    virtual bool hasActualExtentV() const override final { return !m_pParent.expired() && m_pParent.lock()->hasActualExtentV(); }
    virtual VkFormat getActualFormatV() const override final { _ASSERTE(hasActualFormatV()); return m_pParent.lock()->getActualFormatV(); }
    virtual VkExtent2D getActualExtentV() const override final { _ASSERTE(hasActualExtentV()); return m_pParent.lock()->getActualExtentV();
    }
    virtual bool hasActualNumV() const override final { return !m_pParent.expired() && m_pParent.lock()->hasActualNumV(); }
    virtual size_t getActualNumV() const override final { return m_pParent.lock()->getActualNumV(); }

    virtual bool isStandaloneSourceV() const override { return false; }
};

struct SPortDescriptor
{
    enum class EPortType
    {
        INPUT,
        OUTPUT,
        INPUT_OUTPUT
    };

    struct SPortDescription
    {
        EPortType Type;
        std::string Name;
        SPortInfo Format;
    };

    std::vector<SPortDescription> PortDescSet;

    void addInput(const std::string& vName, const SPortInfo& vInfo = SPortInfo::AnyPortInfo);
    void addOutput(const std::string& vName, const SPortInfo& vInfo = SPortInfo::AnyPortInfo);
    void addInputOutput(const std::string& vName, const SPortInfo& vInfo = SPortInfo::AnyPortInfo);
    void clear();
    bool has(EPortType vType, const std::string& vName) const;
    bool hasInput(const std::string& vName) const;
    bool hasOutput(const std::string& vName) const;
    bool hasInputOutput(const std::string& vName) const;
};

class CPortSet
{
public:
    _DEFINE_PTR(CPortSet);

    CPortSet() = delete;
    CPortSet(const SPortDescriptor& vDesc);

    bool isReady() const;
    void assertReady() const;
    bool isInputReady() const;
    void assertInputReady() const;

    bool hasInput(const std::string& vName) const;
    bool hasOutput(const std::string& vName) const;

    size_t getInputPortNum() const;
    size_t getOutputPortNum() const;
    CPort::Ptr getInputPort(size_t vIndex) const;
    CPort::Ptr getOutputPort(size_t vIndex) const;
    CPort::Ptr getInputPort(const std::string& vName) const; // if not found, throw exception
    CPort::Ptr getOutputPort(const std::string& vName) const; // if not found, throw exception
    const SPortInfo& getInputFormat(const std::string& vName) const; // if not found, throw exception
    const SPortInfo& getOutputFormat(const std::string& vName) const; // if not found, throw exception

    void setOutput(const std::string& vOutputName, const std::vector<VkImageView> vImageSet, VkFormat vFormat, VkExtent2D vExtent, VkImageLayout vLayout);
    void setOutput(const std::string& vOutputName, const std::vector<vk::CImage::Ptr>& vImageSet);
    void append(const std::string& vInputName, CPort::Ptr vPort);
    void attachTo(const std::string& vInputName, CPort::Ptr vPort);
    
    void unlinkAll(); // remove all parent of input and children of output

    static void link(CPort::Ptr vOutputPort, CPort::Ptr vInputPort);
    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPort::Ptr vInputPort);
    static void link(CPort::Ptr vOutputPort, CPortSet::Ptr vSet2, const std::string& vInputName);
    static void link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPortSet::Ptr vSet2, const std::string& vInputName);
    
private:
    void __addInput(const std::string& vName, const SPortInfo& vInfo);
    void __addOutput(const std::string& vName, const SPortInfo& vInfo);
    void __addInputOutput(const std::string& vName, const SPortInfo& vInfo);

    CPort::Ptr __findPort(const std::string& vName, const std::vector<CPort::Ptr>& vPortSet) const;
    
    std::vector<CPort::Ptr> m_InputPortSet;
    std::vector<CPort::Ptr> m_OutputPortSet;
};
