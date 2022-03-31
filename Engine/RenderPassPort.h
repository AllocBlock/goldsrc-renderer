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
    vk::CImage::Ptr Image = nullptr;
    EPortType Type;
    size_t Index = 0;

    bool operator==(const SLink& v)
    {
        return TargetName == v.TargetName && Type == v.Type && Index == v.Index;
    }
};

class CRenderPassLink
{
public:
    CRenderPassLink() = delete;
    CRenderPassLink(CRenderPassPort vPorts) : m_Ports(vPorts) {}

    void link(std::string vTargetName, vk::CImage::Ptr vImage, EPortType vType, size_t vIndex = 0);
    void link(const SLink& vLink);
    vk::CImage::Ptr getImage(std::string vTargetName, EPortType vType, size_t vIndex = 0);
    vk::CImage::Ptr getInput(std::string vTargetName, size_t vIndex = 0);
    vk::CImage::Ptr getOutput(std::string vTargetName, size_t vIndex = 0);

private:
    const CRenderPassPort& m_Ports;
    std::vector<SLink> m_LinkSet;
};
