#pragma once
#include "PchVulkan.h"
#include "Image.h"

#include <string>
#include <vector>
#include <map>
#include <functional>

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
        return m_Format.isMatch(vPort->getFormat());
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

    COutputPort(const SPortFormat& vFormat) : CPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override = 0;
    virtual void setImageV(VkImageView vImage, size_t vIndex = 0) = 0;
};

class CInputPort : public CPort
{
public:
    _DEFINE_PTR(CInputPort);

    CInputPort(const SPortFormat& vFormat) : CPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        return m_pTargetPort->getImageV(vIndex);
    }

    void link(COutputPort::Ptr vPort)
    {
        _ASSERTE(isMatch(vPort));
        m_Format = vPort->getFormat(); // FIXME: should I do this?
        m_pTargetPort = vPort;
        vPort->hookUpdate([=] { _triggerUpdate(); });
    }

private:
    COutputPort::Ptr m_pTargetPort = nullptr;
};


class CNormalOutputPort : public COutputPort
{
public:
    _DEFINE_PTR(CNormalOutputPort);

    CNormalOutputPort(const SPortFormat& vFormat) : COutputPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        _ASSERTE(m_ImageMap.find(vIndex) != m_ImageMap.end());
        return m_ImageMap.at(vIndex);
    }

    virtual void setImageV(VkImageView vImage, size_t vIndex = 0) override final
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

class CInputSrcOutputPort : public COutputPort
{
public:
    _DEFINE_PTR(CInputSrcOutputPort);

    CInputSrcOutputPort(const SPortFormat& vFormat = SPortFormat::AnyFormat) : COutputPort(vFormat) {}

    virtual VkImageView getImageV(size_t vIndex = 0) const override final
    {
        return m_pTargetPort->getImageV(vIndex);
    }

    virtual void setImageV(VkImageView vImage, size_t vIndex = 0) override final
    {
        // cant set image for input src output
        throw std::runtime_error("Can't set");
        return;
    }

    void link(CInputPort::Ptr vPort)
    {
        _ASSERTE(isMatch(vPort));
        m_Format = vPort->getFormat(); // FIXME: should I do this?
        m_pTargetPort = vPort;
        vPort->hookUpdate([=] { _triggerUpdate(); });
    }

private:
    CInputPort::Ptr m_pTargetPort = nullptr;
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

    CInputPort::Ptr getInputPort(const std::string& vName)
    {
        _ASSERTE(hasInput(vName));
        return m_InputPortMap.at(vName);
    }

    COutputPort::Ptr getOutputPort(const std::string& vName)
    {
        _ASSERTE(hasOutput(vName));
        return m_OutputPortMap.at(vName);
    }

    void setOutput(const std::string& vOutputName, VkImageView vImage, const SPortFormat& vFormat, size_t vIndex = 0)
    {
        // TODO: how to store actual format in port? or is this important?
        auto pPort = getOutputPort(vOutputName);
        _ASSERTE(vFormat.isMatch(pPort->getFormat()));
        pPort->setImageV(vImage, vIndex);
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

    void linkTo(const std::string& vInputName, COutputPort::Ptr vPort)
    {
        auto pPort = getInputPort(vInputName);
        pPort->link(vPort);
    }

private:
    void __addInput(const std::string& vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasInput(vName));
        m_InputPortMap[vName] = make<CInputPort>(vFormat);
    }

    void __addOutput(const std::string& vName, const SPortFormat& vFormat)
    {
        _ASSERTE(!hasOutput(vName));
        m_OutputPortMap[vName] = make<CNormalOutputPort>(vFormat);
    }

    void __addInputSrcOutput(const std::string& vName, const std::string& vInputName)
    {
        _ASSERTE(!hasOutput(vName));
        _ASSERTE(hasInput(vInputName));
        CInputSrcOutputPort::Ptr pPort = make<CInputSrcOutputPort>();
        pPort->link(m_InputPortMap.at(vInputName));
        m_OutputPortMap[vName] = pPort;
    }

    std::map<std::string, CInputPort::Ptr> m_InputPortMap;
    std::map<std::string, COutputPort::Ptr> m_OutputPortMap;
};