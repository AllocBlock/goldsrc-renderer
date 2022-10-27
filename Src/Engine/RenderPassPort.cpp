#include "Log.h"
#include "RenderPassPort.h"

const SPortFormat& SPortFormat::AnyPortFormat = SPortFormat::createAnyOfUsage(EUsage::DONT_CARE);
const VkFormat& SPortFormat::AnyFormat = VkFormat::VK_FORMAT_UNDEFINED;
const VkExtent2D& SPortFormat::AnyExtent = {0, 0};
const size_t SPortFormat::AnyNum = 0;

VkImageLayout toLayout(EUsage vUsage, bool isDepth)
{
    switch (vUsage)
    {
    case EUsage::DONT_CARE: throw std::runtime_error("Error: can not convert DONT_CARE usage to layout");
    case EUsage::UNDEFINED: return VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    case EUsage::READ: return VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case EUsage::WRITE: return isDepth ? VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case EUsage::PRESENTATION: return VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default: throw std::runtime_error("Error: unknown usage");
    }
}

bool SPortFormat::isMatch(const SPortFormat& v) const
{
    return (Format == v.Format || Format == VkFormat::VK_FORMAT_UNDEFINED || v.Format == VkFormat::VK_FORMAT_UNDEFINED)
        && (Extent.width == v.Extent.width || Extent.width == 0 || v.Extent.width == 0)
        && (Extent.height == v.Extent.height || Extent.height == 0 || v.Extent.height == 0)
        && (Num == v.Num || Num == 0 || v.Num == 0);
}

SPortFormat SPortFormat::createAnyOfUsage(EUsage vUsage)
{
    return { VkFormat::VK_FORMAT_UNDEFINED, {0, 0}, 0, vUsage };
}

void ILinkEvent::_onImageUpdate()
{
    m_ImageUpdateEventHandler.trigger();
    _onImageUpdateExtendV();
}

void ILinkEvent::_onLinkUpdate(EventId_t vEventId, ILinkEvent::CPtr vFrom)
{
    m_LinkUpdateEventHandler.trigger(vEventId, vFrom);
    _onLinkUpdateExtendV(vEventId, vFrom);
}

void CPort::removeParent()
{
    if (!m_pParent.expired())
    {
        // remove from parent
        auto& ChildSet = m_pParent.lock()->m_ChildSet;
        auto pThis = shared_from_this();
        auto pIterIndex = std::find(ChildSet.begin(), ChildSet.end(), pThis);
        if (pIterIndex != ChildSet.end())
            ChildSet.erase(pIterIndex);
    }
    m_pParent.reset();
}

void CPort::clearAllChildren()
{
    for (auto pChild : m_ChildSet)
        pChild->m_pParent.reset();
    m_ChildSet.clear();
}

void CPort::attachTo(CPort::Ptr vPort)
{
    _ASSERTE(isMatch(vPort));

    auto pThis = shared_from_this();

    m_pParent = vPort;
    vPort->m_ChildSet.emplace_back(pThis);

    _onLinkUpdate(m_LinkUpdateEventHandler.generateEventId(), nullptr);
}

void CPort::unlinkAll()
{
    removeParent();
    clearAllChildren();
}

void CPort::setForceNotReady(bool vForceNotReady)
{ 
    if (m_ForceNotReady != vForceNotReady)
    {
        m_ForceNotReady = vForceNotReady;
        _onImageUpdate();
        _onLinkUpdate(m_LinkUpdateEventHandler.generateEventId(), nullptr);
    }
}

void CPort::_onImageUpdateExtendV()
{
    for (const auto& pChild : m_ChildSet)
        pChild->_onImageUpdate();
}

void CPort::_onLinkUpdateExtendV(EventId_t vEventId, ILinkEvent::CPtr vFrom)
{
    auto pThis = shared_from_this();
    if (!m_pParent.expired() && m_pParent.lock() != vFrom)
        m_pParent.lock()->_onLinkUpdate(vEventId, pThis);

    for (auto pChild : m_ChildSet)
    {
        if (pChild != vFrom)
            pChild->_onLinkUpdate(vEventId, pThis);
    }
}

CSourcePort::CSourcePort(const std::string& vName, const SPortFormat& vFormat): CPort(vName, vFormat)
{
    if (vFormat.Format != VkFormat::VK_FORMAT_UNDEFINED)
    {
        setActualFormat(vFormat.Format);
    }

    if (vFormat.Extent.width != 0 && vFormat.Extent.height != 0)
    {
        setActualExtent(vFormat.Extent);
    }
}

VkImageView CSourcePort::getImageV(size_t vIndex) const
{
    _ASSERTE(m_ImageMap.find(vIndex) != m_ImageMap.end());
    return m_ImageMap.at(vIndex);
}

bool CSourcePort::isLinkReadyV() const
{
    return CPort::isLinkReadyV();
}

bool CSourcePort::isImageReadyV() const
{
    // has actual format and extent, image are all set
    return CPort::isImageReadyV() && hasActualFormatV() && hasActualExtentV() && (m_Format.Num == 0 || m_Format.Num == m_ImageMap.size());
}

void CSourcePort::setImage(VkImageView vImage, size_t vIndex)
{
    if (m_ImageMap.find(vIndex) == m_ImageMap.end() || m_ImageMap[vIndex] != vImage)
    {
        m_ImageMap[vIndex] = vImage;
        _onImageUpdate();
    }
}

VkImageView CRelayPort::getImageV(size_t vIndex) const
{
    _ASSERTE(!m_pParent.expired());
    return m_pParent.lock()->getImageV(vIndex);
}

bool CRelayPort::isLinkReadyV() const
{
    if (m_pParent.expired()) return false;
    else return CPort::isLinkReadyV() && m_pParent.lock()->isLinkReadyV();
}

bool CRelayPort::isImageReadyV() const
{
    return CPort::isImageReadyV() && isLinkReadyV() && hasParent() && m_pParent.lock()->isImageReadyV();
}

void SPortDescriptor::addInput(std::string vName, const SPortFormat& vFormat)
{
    _ASSERTE(!hasInput(vName));
    _ASSERTE(!hasInputOutput(vName));

    m_InputPortNameSet.emplace_back(vName);
    m_InputPortSet.emplace_back(vFormat);
}

void SPortDescriptor::addOutput(std::string vName, const SPortFormat& vFormat)
{
    _ASSERTE(!hasOutput(vName));
    _ASSERTE(!hasInputOutput(vName));

    m_OutputPortNameSet.emplace_back(vName);
    m_OutputPortSet.emplace_back(vFormat);
}

void SPortDescriptor::addInputOutput(std::string vName, const SPortFormat& vFormat)
{
    _ASSERTE(!hasInput(vName));
    _ASSERTE(!hasOutput(vName));
    _ASSERTE(!hasInputOutput(vName));

    m_InputOutputPortNameSet.emplace_back(vName);
    m_InputOutputPortSet.emplace_back(vFormat);
}

void SPortDescriptor::clear()
{
    m_InputPortNameSet.clear();
    m_InputPortSet.clear();
    m_OutputPortNameSet.clear();
    m_OutputPortSet.clear();
    m_InputOutputPortNameSet.clear();
    m_InputOutputPortSet.clear();
}

bool SPortDescriptor::hasInput(const std::string vName) const
{ 
    return std::find(m_InputPortNameSet.begin(), m_InputPortNameSet.end(), vName) != m_InputPortNameSet.end();
}

bool SPortDescriptor::hasOutput(const std::string vName) const
{
    return std::find(m_OutputPortNameSet.begin(), m_OutputPortNameSet.end(), vName) != m_OutputPortNameSet.end();
}

bool SPortDescriptor::hasInputOutput(const std::string vName) const
{
    return std::find(m_InputOutputPortNameSet.begin(), m_InputOutputPortNameSet.end(), vName) != m_InputOutputPortNameSet.end();
}

CPortSet::CPortSet(const SPortDescriptor& vDesc)
{
    m_pImageUpdateCallbackFunc = [this]()
    {
        m_InputImageUpdateEventHandler.trigger();
    };
    m_pLinkUpdateCallbackFunc = [this](EventId_t vEventId, ILinkEvent::CPtr vFrom)
    {
        if (this->m_LastEventId != vEventId) // only once
        {
            this->m_LastEventId = vEventId;
            m_LinkUpdateEventHandler.trigger(vEventId, vFrom);
        }
    };

    for (size_t i = 0; i < vDesc.m_InputPortNameSet.size(); ++i)
    {
        __addInput(vDesc.m_InputPortNameSet[i], vDesc.m_InputPortSet[i]);
    }
    for (size_t i = 0; i < vDesc.m_OutputPortNameSet.size(); ++i)
    {
        __addOutput(vDesc.m_OutputPortNameSet[i], vDesc.m_OutputPortSet[i]);
    }

    for (size_t i = 0; i < vDesc.m_InputOutputPortNameSet.size(); ++i)
    {
        __addInputOutput(vDesc.m_InputOutputPortNameSet[i], vDesc.m_InputOutputPortSet[i]);
    }
}

bool CPortSet::isLinkReady()
{
    for (const auto& pPort : m_InputPortSet)
        if (!pPort->isLinkReadyV()) return false;
    for (const auto& pPort : m_OutputPortSet)
        if (!pPort->isLinkReadyV()) return false;
    return true;
}

bool CPortSet::isImageReady()
{
    for (const auto& pPort : m_InputPortSet)
        if (!pPort->isImageReadyV()) return false;
    for (const auto& pPort : m_OutputPortSet)
        if (!pPort->isImageReadyV()) return false;
    return true;
}

size_t CPortSet::getInputPortNum() const { return m_InputPortSet.size(); }
size_t CPortSet::getOutputPortNum() const { return m_OutputPortSet.size(); }

bool CPortSet::hasInput(const std::string& vName) const
{ return __findPort(vName, m_InputPortSet) != nullptr; }

bool CPortSet::hasOutput(const std::string& vName) const
{ return __findPort(vName, m_OutputPortSet) != nullptr; }

CPort::Ptr CPortSet::getInputPort(size_t vIndex) const
{
    _ASSERTE(vIndex < m_InputPortSet.size());
    return m_InputPortSet[vIndex];
}

CPort::Ptr CPortSet::getOutputPort(size_t vIndex) const
{
    _ASSERTE(vIndex < m_OutputPortSet.size());
    return m_OutputPortSet[vIndex];
}

CPort::Ptr CPortSet::getInputPort(const std::string& vName) const
{
    auto pPort = __findPort(vName, m_InputPortSet);
    _ASSERTE(pPort);
    return pPort;
}

CPort::Ptr CPortSet::getOutputPort(const std::string& vName) const
{
    auto pPort = __findPort(vName, m_OutputPortSet);
    _ASSERTE(pPort);
    return pPort;
}

const SPortFormat& CPortSet::getInputFormat(const std::string& vName) const
{
    return getInputPort(vName)->getFormat();
}

const SPortFormat& CPortSet::getOutputFormat(const std::string& vName) const
{
    return getOutputPort(vName)->getFormat();
}

void CPortSet::setOutput(const std::string& vOutputName, VkImageView vImage, VkFormat vFormat, VkExtent2D vExtent,
    size_t vIndex)
{
    SPortFormat Format;
    Format.Format = vFormat;
    Format.Extent = vExtent;
    Format.Num = 0; // dont care

    auto pPort = getOutputPort(vOutputName);
    const auto& TargetFormat = pPort->getFormat();
    _ASSERTE(Format.isMatch(pPort->getFormat()));

    if (TargetFormat.Num != 0 && vIndex >= TargetFormat.Num)
    {
        Log::log("Warning: more images than specific port image num are set on Output port [" + vOutputName + "]");
    }

    CSourcePort::Ptr pSourcePort = std::dynamic_pointer_cast<CSourcePort>(pPort);

    // FIXME: what if different formats/extents are set? how to deal this?
    pSourcePort->setActualFormat(vFormat);
    pSourcePort->setActualExtent(vExtent);
    pSourcePort->setImage(vImage, vIndex);
}

void CPortSet::setOutput(const std::string& vOutputName, const vk::CImage& vImage, size_t vIndex)
{
    setOutput(vOutputName, vImage, vImage.getFormat(), vImage.getExtent(), vIndex);
}

void CPortSet::append(const std::string& vInputName, CPort::Ptr vPort)
{
    auto pPort = getInputPort(vInputName);
    pPort->append(vPort);
}

void CPortSet::attachTo(const std::string& vInputName, CPort::Ptr vPort)
{
    auto pPort = getInputPort(vInputName);
    pPort->attachTo(vPort);
}

void CPortSet::unlinkAll()
{
    for (auto pPort : m_InputPortSet)
        pPort->unlinkAll();
    for (auto pPort : m_OutputPortSet)
        pPort->unlinkAll();
}

void CPortSet::link(CPort::Ptr vOutputPort, CPort::Ptr vInputPort)
{
    _ASSERTE(vOutputPort);
    _ASSERTE(vInputPort);
    vOutputPort->append(vInputPort);
}

void CPortSet::link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPort::Ptr vInputPort)
{
    link(vSet1->getOutputPort(vOutputName), vInputPort);
}

void CPortSet::link(CPort::Ptr vOutputPort, CPortSet::Ptr vSet2, const std::string& vInputName)
{
    link(vOutputPort, vSet2->getInputPort(vInputName));
}

void CPortSet::link(CPortSet::Ptr vSet1, const std::string& vOutputName, CPortSet::Ptr vSet2,
    const std::string& vInputName)
{
    link(vSet1->getOutputPort(vOutputName), vSet2->getInputPort(vInputName));
}

void CPortSet::__addInput(const std::string& vName, const SPortFormat& vFormat)
{
    _ASSERTE(!hasInput(vName));
    auto pPort = make<CRelayPort>(vName, vFormat);
    pPort->hookImageUpdate(m_pImageUpdateCallbackFunc);
    pPort->hookLinkUpdate(m_pLinkUpdateCallbackFunc);
    m_InputPortSet.emplace_back(pPort);
}

void CPortSet::__addOutput(const std::string& vName, const SPortFormat& vFormat)
{
    _ASSERTE(!hasOutput(vName));
    auto pPort = make<CSourcePort>(vName, vFormat);
    pPort->hookLinkUpdate(m_pLinkUpdateCallbackFunc);
    m_OutputPortSet.emplace_back(pPort);
}

void CPortSet::__addInputOutput(const std::string& vName, const SPortFormat& vFormat)
{
    _ASSERTE(!hasOutput(vName));
    _ASSERTE(!hasInput(vName));
    auto pPort = make<CRelayPort>(vName, vFormat);
    pPort->hookImageUpdate(m_pImageUpdateCallbackFunc);
    pPort->hookLinkUpdate(m_pLinkUpdateCallbackFunc);
    m_InputPortSet.emplace_back(pPort);
    m_OutputPortSet.emplace_back(pPort);
}

CPort::Ptr CPortSet::__findPort(const std::string& vName, const std::vector<CPort::Ptr>& vPortSet) const
{
    for (const auto& pPort : vPortSet)
        if (pPort->getName() == vName)
            return pPort;
    return nullptr;
}
