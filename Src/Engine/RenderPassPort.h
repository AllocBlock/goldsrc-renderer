#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "Image.h"

struct SPort
{
    std::string Name;
    VkFormat Format = VkFormat::VK_FORMAT_UNDEFINED;
    VkExtent2D Extent = { 0, 0 };

    bool operator==(const SPort& v)
    {
        return Name == v.Name && Format == v.Format && Extent.width == v.Extent.width && Extent.height == v.Extent.height;
    }
};

class CRenderPassPort
{
public:
    void addInput(std::string vName, VkFormat vFormat, VkExtent2D vExtent);
    void addInput(const SPort& vPort);
    void addOutput(std::string vName, VkFormat vFormat, VkExtent2D vExtent);
    void addOutput(const SPort& vPort);
    void clear();

    bool hasInput() const { return !m_InputSet.empty(); }
    bool hasOutput() const { return !m_OutputSet.empty(); }

private:
    std::vector<SPort> m_InputSet;
    std::vector<SPort> m_OutputSet;
};

enum class EPortType
{
    INPUT,
    OUTPUT
};

struct SLink
{
    std::string TargetName;
    VkImageView ImageView = nullptr;
    EPortType Type;
    size_t Index = 0;

    bool operator==(const SLink& v) const
    {
        return TargetName == v.TargetName && Index == v.Index;
    }

    bool isMatch(const SLink& v) const
    {
        return TargetName == v.TargetName && Type == v.Type && Index == v.Index;
    }
};

class CRenderPassLink
{
public:
    _DEFINE_PTR(CRenderPassLink);

    CRenderPassLink() = delete;
    CRenderPassLink(const CRenderPassPort& vPorts) : m_Ports(vPorts) {}

    void link(const SLink& vLink);
    void link(std::string vTargetName, VkImageView vImageView, EPortType vType, size_t vIndex = 0);
    void linkInput(std::string vTargetName, VkImageView vImageView, size_t vIndex = 0) { return link(vTargetName, vImageView, EPortType::INPUT, vIndex); }
    void linkOutput(std::string vTargetName, VkImageView vImageView, size_t vIndex = 0) { return link(vTargetName, vImageView, EPortType::OUTPUT, vIndex); }
    void unlink(const SLink& vLink);
    void unlink(std::string vTargetName, EPortType vType, size_t vIndex = 0);
    bool hasLink(const SLink& vLink) const;
    bool hasLink(std::string vTargetName, EPortType vType, size_t vIndex = 0) const;
    VkImageView getImage(std::string vTargetName, EPortType vType, size_t vIndex = 0) const;
    VkImageView getInput(std::string vTargetName, size_t vIndex = 0) const;
    VkImageView getOutput(std::string vTargetName, size_t vIndex = 0) const;

    bool isUpdated() const { return m_Updated; }
    void setUpdateState(bool vState) { m_Updated = vState; }

private:
    bool __findLink(const SLink& vLink, size_t& voIndex) const;

    const CRenderPassPort& m_Ports;
    std::vector<SLink> m_LinkSet;
    bool m_Updated = false;
};
