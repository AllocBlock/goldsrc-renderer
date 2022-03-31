#include "RenderPassPort.h"

void CRenderPassPort::addInput(std::string vName, VkFormat vFormat, VkExtent2D vExtent)
{
    _ASSERTE(!vName.empty() && vFormat != VK_FORMAT_UNDEFINED && vExtent.width != 0 && vExtent.height != 0);
    m_InputSet.push_back({ vName, vFormat, vExtent });
}

void CRenderPassPort::addInput(const SPort& vPort)
{
    _ASSERTE(!vPort.Name.empty() && vPort.Format != VK_FORMAT_UNDEFINED && vPort.Extent.width != 0 && vPort.Extent.height != 0);
    m_InputSet.emplace_back(vPort);
}

void CRenderPassPort::addOutput(std::string vName, VkFormat vFormat, VkExtent2D vExtent)
{
    _ASSERTE(!vName.empty() && vFormat != VK_FORMAT_UNDEFINED && vExtent.width != 0 && vExtent.height != 0);
    m_OutputSet.push_back({ vName, vFormat, vExtent });
}

void CRenderPassPort::addOutput(const SPort& vPort)
{
    _ASSERTE(!vPort.Name.empty() && vPort.Format != VK_FORMAT_UNDEFINED && vPort.Extent.width != 0 && vPort.Extent.height != 0);
    m_OutputSet.emplace_back(vPort);
}

void CRenderPassPort::clear()
{
    m_InputSet.clear();
    m_OutputSet.clear();
}

void CRenderPassLink::link(std::string vTargetName, vk::CImage::Ptr vImage, EPortType vType, size_t vIndex)
{
    _ASSERTE(vImage);
    m_LinkSet.push_back({ vTargetName, vImage, vType, vIndex });
}

void CRenderPassLink::link(const SLink& vLink)
{
    _ASSERTE(vLink.Image);
    m_LinkSet.emplace_back(vLink);
}


vk::CImage::Ptr CRenderPassLink::getImage(std::string vTargetName, EPortType vType, size_t vIndex)
{
    _ASSERTE(!vTargetName.empty());
    _ASSERTE(vIndex < m_LinkSet.size());
    for (const SLink& Link : m_LinkSet)
    {
        if (Link.Type != vType) continue;
        if (Link.TargetName == vTargetName && Link.Index == vIndex)
            return Link.Image;
    }
    throw std::runtime_error(u8"Î´ÕÒµ½¸Ã¶Ë¿Ú");
    return nullptr;
}

vk::CImage::Ptr CRenderPassLink::getInput(std::string vTargetName, size_t vIndex)
{
    return getImage(vTargetName, EPortType::INPUT, vIndex);
}

vk::CImage::Ptr CRenderPassLink::getOutput(std::string vTargetName, size_t vIndex)
{
    return getImage(vTargetName, EPortType::OUTPUT, vIndex);
}