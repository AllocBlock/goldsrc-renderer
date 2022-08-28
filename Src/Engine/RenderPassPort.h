#pragma once
#include "Pointer.h"
#include "Image.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <map>

struct SPortFormat
{
    VkFormat Format = VkFormat::VK_FORMAT_UNDEFINED;
    VkExtent2D Extent = { 0, 0 };
    size_t Num = 1;

    bool operator==(const SPortFormat& v)
    {
        return Format == v.Format && Extent.width == v.Extent.width && Extent.height == v.Extent.height && Num == v.Num;
    }
};

class CPort
{
public:
    _DEFINE_PTR(CPort);

    CPort() = delete;
    CPort(const SPortFormat& vFormat): m_Format(vFormat) {}

    const SPortFormat& getFormat() const { return m_Format; }
    virtual VkImageView getImageV(size_t vIndex = 0) const = 0;

    bool isMatch(CPort::CPtr vPort)
    {
        return m_Format == vPort->getFormat();
    }

    void hookUpdate(std::function<void()> vCallback)
    {
        m_HookSet.emplace_back(vCallback);
    }
protected:
    void _triggerUpdate()
    {
        for (const auto& Func : m_HookSet)
            Func();
    }

    SPortFormat m_Format;

private:
    std::vector<std::function<void()>> m_HookSet;
};

class COutputPort : public CPort
{
public:
    _DEFINE_PTR(COutputPort);

    COutputPort(const SPortFormat& vFormat) : CPort(vFormat) 
    {
        m_ImageSet.resize(vFormat.Num, VK_NULL_HANDLE);
    }

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        _ASSERTE(vIndex < m_ImageSet.size());
        _ASSERTE(m_ImageSet[vIndex] != VK_NULL_HANDLE);
        return m_ImageSet[vIndex];
    }

    void setImage(VkImageView vImage, size_t vIndex = 0)
    {
        _ASSERTE(vIndex < m_ImageSet.size());
        if (m_ImageSet[vIndex] != vImage)
        {
            m_ImageSet[vIndex] = vImage;
            _triggerUpdate();
        }
    }

private:
    std::vector<VkImageView> m_ImageSet;
};


class CInputPort : public CPort
{
public:
    _DEFINE_PTR(CInputPort);

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        return m_pTargetPort->getImageV(vIndex);
    }

    void link(COutputPort::Ptr vPort)
    {
        _ASSERTE(isMatch(vPort));
        m_pTargetPort = vPort;
        vPort->hookUpdate([=] { _triggerUpdate(); });
    }

private:
    COutputPort::Ptr m_pTargetPort;
};

class SPortDescriptor
{
public:
    void addInput(std::string vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasInput(vName));
        m_InputPortNameSet.emplace_back(vName);
        m_InputPortSet.emplace_back(vFormat);
    }

    void addOutput(std::string vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasOutput(vName));
        m_OutputPortNameSet.emplace_back(vName);
        m_OutputPortSet.emplace_back(vFormat);
    }

    void clear()
    {
        m_InputPortNameSet.clear();
        m_InputPortSet.clear();
        m_OutputPortNameSet.clear();
        m_OutputPortSet.clear();
    }

    bool hasInput(const std::string vName) const 
    { 
        return std::find(m_InputPortNameSet.begin(), m_InputPortNameSet.end(), vName) != m_InputPortNameSet.end();
    }
    bool hasOutput(const std::string vName) const
    {
        return std::find(m_OutputPortNameSet.begin(), m_OutputPortNameSet.end(), vName) != m_OutputPortNameSet.end();
    }

    std::vector<std::string> m_InputPortNameSet;
    std::vector<SPortFormat> m_InputPortSet;
    std::vector<std::string> m_OutputPortNameSet;
    std::vector<SPortFormat> m_OutputPortSet;
};

class CPortSet
{
public:
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
    }

    bool hasInput(const std::string vName) const { return m_InputPortMap.find(vName) != m_InputPortMap.end(); }
    bool hasOutput(const std::string vName) const { return m_OutputPortMap.find(vName) != m_OutputPortMap.end(); }

    CInputPort::Ptr getInputPort(std::string vName)
    {
        _ASSERTE(hasInput(vName));
        return m_InputPortMap[vName];
    }

    COutputPort::Ptr getOutputPort(std::string vName)
    {
        _ASSERTE(hasOutput(vName));
        return m_OutputPortMap[vName];
    }

    void setOutput(std::string vOutputName, VkImageView vImage, size_t vIndex = 0)
    {
        auto pPort = getOutputPort(vOutputName);
        pPort->setImage(vImage, vIndex);
    }

    void linkTo(std::string vInputName, COutputPort::Ptr vPort)
    {
        auto pPort = getInputPort(vInputName);
        pPort->link(vPort);
    }

private:
    void __addInput(std::string vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasInput(vName));
        m_InputPortMap[vName] = make<CInputPort>(vFormat);
    }

    void __addOutput(std::string vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasOutput(vName));
        m_OutputPortMap[vName] = make<COutputPort>(vFormat);
    }

    std::map<std::string, CInputPort::Ptr> m_InputPortMap;
    std::map<std::string, COutputPort::Ptr> m_OutputPortMap;
};