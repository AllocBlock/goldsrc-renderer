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
    
    CPort() = delete;
    CPort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet);
    virtual ~CPort() = default;
    
    _DEFINE_GETTER(Name, std::string)
    void setName(const std::string& vName) { _ASSERTE(!vName.empty()); m_Name = vName; }

    bool isRoot() const { return !hasParent(); }
    bool isTail() const { return !hasChildren(); }
    bool hasParent() const { return !m_pParent.expired(); }
    sptr<CPort> getParent() const { return m_pParent.expired() ? nullptr : m_pParent.lock(); }
    void removeParent();
    bool hasChildren() const { return !m_ChildSet.empty(); }
    size_t getChildNum() const { return m_ChildSet.size(); }
    sptr<CPort> getChild(size_t vIndex) const { return m_ChildSet[vIndex]; }
    void append(sptr<CPort> vPort) { vPort->attachTo(shared_from_this()); }
    void clearAllChildren();
    void attachTo(sptr<CPort> vPort);
    void unlinkAll();
    
    bool hasLayout() const;
    VkImageLayout getLayout() const;
    void setLayout(VkImageLayout vLayout);

    bool isReady() const;
    void assertReady() const;

    const SPortInfo& getInfo() const { return m_Info; }
    virtual sptr<vk::CImage> getImageV() const = 0;
    bool isMatch(cptr<CPort> vPort) const { return m_Info.isMatch(vPort->getInfo()); }
    
    CPortSet* getBelongedPortSet() const { return m_pBelongedSet; }

protected:
    std::string m_Name = "";
    CPortSet* const m_pBelongedSet;
    wptr<CPort> m_pParent;
    std::vector<sptr<CPort>> m_ChildSet;
    
    SPortInfo m_Info;
    std::optional<VkImageLayout> m_Layout = std::nullopt;
};

class CSourcePort : public CPort
{
public:
    
    CSourcePort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet);

    virtual sptr<vk::CImage> getImageV() const override final;
    void setImage(sptr<vk::CImage> vImage);
    void clearImage() { m_pImage = nullptr; }

private:
    sptr<vk::CImage> m_pImage;
    
    VkFormat m_ActualFormat = SPortInfo::AnyFormat;
    VkExtent2D m_ActualExtent = SPortInfo::AnyExtent;
};

class CRelayPort : public CPort
{
public:
    
    CRelayPort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet);

    virtual sptr<vk::CImage> getImageV() const override final;
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
    sptr<CPort> getInputPort(size_t vIndex) const;
    sptr<CPort> getOutputPort(size_t vIndex) const;
    sptr<CPort> getInputPort(const std::string& vName) const; // if not found, throw exception
    sptr<CPort> getOutputPort(const std::string& vName) const; // if not found, throw exception
    const SPortInfo& getInputPortInfo(const std::string& vName) const; // if not found, throw exception
    const SPortInfo& getOutputPortInfo(const std::string& vName) const; // if not found, throw exception

    void setOutput(const std::string& vOutputName, sptr<vk::CImage> vImage);
    void append(const std::string& vInputName, sptr<CPort> vPort);
    void attachTo(const std::string& vInputName, sptr<CPort> vPort);
    
    void unlinkAll(); // remove all parent of input and children of output

    static void link(sptr<CPort> vOutputPort, sptr<CPort> vInputPort);
    static void link(sptr<CPortSet> vSet1, const std::string& vOutputName, sptr<CPort> vInputPort);
    static void link(sptr<CPort> vOutputPort, sptr<CPortSet> vSet2, const std::string& vInputName);
    static void link(sptr<CPortSet> vSet1, const std::string& vOutputName, sptr<CPortSet> vSet2, const std::string& vInputName);
    
private:
    void __addInput(const std::string& vName, const SPortInfo& vInfo);
    void __addOutput(const std::string& vName, const SPortInfo& vInfo);
    void __addInputOutput(const std::string& vName, const SPortInfo& vInfo);

    sptr<CPort> __findPort(const std::string& vName, const std::vector<sptr<CPort>>& vPortSet) const;
    
    std::vector<sptr<CPort>> m_InputPortSet;
    std::vector<sptr<CPort>> m_OutputPortSet;
};
