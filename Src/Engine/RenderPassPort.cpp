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

void CRenderPassLink::link(const SLink& vLink)
{
    _ASSERTE(vLink.ImageView != VK_NULL_HANDLE);
    unlink(vLink);
    m_LinkSet.emplace_back(vLink);
    setUpdateState(true);
}

void CRenderPassLink::link(std::string vTargetName, VkImageView vImageView, EPortType vType, size_t vIndex)
{
    link({ vTargetName, vImageView, vType, vIndex });
}

void CRenderPassLink::unlink(const SLink& vLink)
{
    size_t Index;
    if (__findLink(vLink, Index))
        m_LinkSet.erase(m_LinkSet.begin() + Index);
    setUpdateState(true);
}

void CRenderPassLink::unlink(std::string vTargetName, EPortType vType, size_t vIndex)
{
    unlink({ vTargetName, VK_NULL_HANDLE, vType, vIndex });
}

bool CRenderPassLink::hasLink(const SLink& vLink) const
{
    size_t Index;
    return __findLink(vLink, Index);
}

bool CRenderPassLink::hasLink(std::string vTargetName, EPortType vType, size_t vIndex) const
{
    return hasLink({ vTargetName, VK_NULL_HANDLE, vType, vIndex });
}

VkImageView CRenderPassLink::getImage(std::string vTargetName, EPortType vType, size_t vIndex)
{
    _ASSERTE(!vTargetName.empty());
    for (const SLink& Link : m_LinkSet)
    {
        if (Link.Type != vType) continue;
        if (Link.TargetName == vTargetName && Link.Index == vIndex)
            return Link.ImageView;
    }
    throw std::runtime_error(u8"Î´ÕÒµ½¸Ã¶Ë¿Ú");
    return nullptr;
}

VkImageView CRenderPassLink::getInput(std::string vTargetName, size_t vIndex)
{
    return getImage(vTargetName, EPortType::INPUT, vIndex);
}

VkImageView CRenderPassLink::getOutput(std::string vTargetName, size_t vIndex)
{
    return getImage(vTargetName, EPortType::OUTPUT, vIndex);
}

bool CRenderPassLink::__findLink(const SLink& vLink, size_t& voIndex) const
{
    auto pResult = std::find(m_LinkSet.begin(), m_LinkSet.end(), vLink);
    if (pResult != m_LinkSet.end())
    {
        voIndex = std::distance(m_LinkSet.begin(), pResult);
        return true;
    }
    else
        return false;
}