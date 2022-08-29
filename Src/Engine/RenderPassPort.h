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
    COutputPort::Ptr m_pTargetPort;
};

class SPortDescriptor
{
public:
    void addInput(std::string vName, const SPortFormat& vFormat = SPortFormat{ VK_FORMAT_UNDEFINED, {0, 0}, 0 })
    {
        _ASSERTE(!hasInput(vName));
        m_InputPortNameSet.emplace_back(vName);
        m_InputPortSet.emplace_back(vFormat);
    }

    void addOutput(std::string vName, const SPortFormat& vFormat = SPortFormat{ VK_FORMAT_UNDEFINED, {0, 0}, 0 })
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

    void setOutput(std::string vOutputName, VkImageView vImage, const SPortFormat& vFormat, size_t vIndex = 0)
    {
        // TODO: how to store actual format in port? or is this important?
        auto pPort = getOutputPort(vOutputName);
        _ASSERTE(vFormat.isMatch(pPort->getFormat()));
        pPort->setImage(vImage, vIndex);
    }

    void setOutput(std::string vOutputName, vk::CImage::CPtr vImage, size_t vIndex = 0)
    {
        SPortFormat Format;
        Format.Format = vImage->getFormat();
        Format.Extent.width = vImage->getWidth();
        Format.Extent.height = vImage->getHeight();
        Format.Num = 0; // dont care

        setOutput(vOutputName, *vImage, Format, vIndex);
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