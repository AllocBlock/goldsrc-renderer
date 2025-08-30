#include "Log.h"
#include "Debug.h"
#include "RenderPassPort.h"

const SPortInfo& SPortInfo::AnyPortInfo = SPortInfo::createAnyOfUsage(EImageUsage::UNKNOWN);
const VkFormat& SPortInfo::AnyFormat = VkFormat::VK_FORMAT_UNDEFINED;
const VkExtent2D& SPortInfo::AnyExtent = {0, 0};
const size_t SPortInfo::AnyNum = 0;

VkImageLayout __toLayout(EImageUsage vUsage)
{
    switch (vUsage)
    {
    case EImageUsage::UNKNOWN: throw std::runtime_error("Error: can not convert DONT_CARE usage to layout");
    case EImageUsage::READ: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case EImageUsage::COLOR_ATTACHMENT: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case EImageUsage::DEPTH_ATTACHMENT: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case EImageUsage::PRESENTATION: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default: throw std::runtime_error("Error: unknown usage");
    }
}

bool SPortInfo::isMatch(const SPortInfo& v) const
{
    return (Format == v.Format || Format == VkFormat::VK_FORMAT_UNDEFINED || v.Format == VkFormat::VK_FORMAT_UNDEFINED)
        && (Extent.width == v.Extent.width || Extent.width == 0 || v.Extent.width == 0)
        && (Extent.height == v.Extent.height || Extent.height == 0 || v.Extent.height == 0)
        && (Num == v.Num || Num == 0 || v.Num == 0);
}

SPortInfo SPortInfo::createAnyOfUsage(EImageUsage vUsage)
{
    return { VkFormat::VK_FORMAT_UNDEFINED, {0, 0}, 0, vUsage };
}

CPort::CPort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet): m_Info(vInfo), m_pBelongedSet(vBelongedSet)
{
    setName(vName);
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
}

void CPort::unlinkAll()
{
    removeParent();
    clearAllChildren();
}

bool CPort::hasInputLayout() const
{ return m_InputLayout.has_value(); }

VkImageLayout CPort::getInputLayout() const
{
    return m_InputLayout.value();
}

void CPort::setInputLayout(VkImageLayout vLayout)
{
    m_InputLayout = vLayout;
}

bool CPort::hasOutputLayout() const
{ return m_OutputLayout.has_value(); }

VkImageLayout CPort::getOutputLayout() const
{
    return m_OutputLayout.value();
}

void CPort::setOutputLayout(VkImageLayout vLayout)
{
    _ASSERTE(vLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    m_OutputLayout = vLayout;
}

bool CPort::isReady() const
{
    return m_InputLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
        m_OutputLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
        hasActualFormatV() &&
        hasActualExtentV();
}

void CPort::assertReady() const
{
    if (!m_InputLayout.has_value())
        throw std::runtime_error("Input layout is not set");
    if (!m_OutputLayout.has_value() || m_OutputLayout.value() == VK_IMAGE_LAYOUT_UNDEFINED)
        throw std::runtime_error("Output layout is not set or is not valid");
    if (!hasActualFormatV())
        throw std::runtime_error("Port has no actual format");
    if (!hasActualExtentV())
        throw std::runtime_error("Port has no actual extent");
}

CSourcePort::CSourcePort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet) : CPort(vName, vInfo, vBelongedSet)
{
    if (vInfo.Format != VkFormat::VK_FORMAT_UNDEFINED)
    {
        setActualFormat(vInfo.Format);
    }

    if (vInfo.Extent.width != 0 && vInfo.Extent.height != 0)
    {
        setActualExtent(vInfo.Extent);
    }

    m_InputLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

VkImageView CSourcePort::getImageV() const
{
    return m_ImageView;
}

size_t CSourcePort::getImageNumV() const
{
    return m_Info.Num;
}

void CSourcePort::setImage(VkImageView vImageView)
{
    m_ImageView = vImageView;
}

size_t CRelayPort::getImageNumV() const
{
    _ASSERTE(!m_pParent.expired());
    return m_pParent.lock()->getImageNumV();
}

CRelayPort::CRelayPort(const std::string& vName, const SPortInfo& vInfo, CPortSet* vBelongedSet): CPort(vName, vInfo, vBelongedSet)
{
    if (vInfo.Usage != EImageUsage::UNKNOWN)
        setInputLayout(__toLayout(vInfo.Usage));
}

VkImageView CRelayPort::getImageV() const
{
    _ASSERTE(!m_pParent.expired());
    return m_pParent.lock()->getImageV();
}

void SPortDescriptor::addInput(const std::string& vName, const SPortInfo& vInfo)
{
    _ASSERTE(!hasInput(vName));
    _ASSERTE(!hasInputOutput(vName));
    PortDescSet.emplace_back(SPortDescription{ EPortType::INPUT, vName, vInfo });
}

void SPortDescriptor::addOutput(const std::string& vName, const SPortInfo& vInfo)
{
    _ASSERTE(!hasOutput(vName));
    _ASSERTE(!hasInputOutput(vName));
    PortDescSet.emplace_back(SPortDescription{ EPortType::OUTPUT, vName, vInfo });
}

void SPortDescriptor::addInputOutput(const std::string& vName, const SPortInfo& vInfo)
{
    _ASSERTE(!hasInput(vName));
    _ASSERTE(!hasOutput(vName));
    _ASSERTE(!hasInputOutput(vName));
    PortDescSet.emplace_back(SPortDescription{ EPortType::INPUT_OUTPUT, vName, vInfo });
}

void SPortDescriptor::clear()
{
    PortDescSet.clear();
}

bool SPortDescriptor::has(EPortType vType, const std::string& vName) const
{
    for (const SPortDescription& PortDesc : PortDescSet)
    {
        if (vType == PortDesc.Type && vName == PortDesc.Name)
            return true;
    }
    return false;
}

bool SPortDescriptor::hasInput(const std::string& vName) const
{ 
    return has(EPortType::INPUT, vName);
}

bool SPortDescriptor::hasOutput(const std::string& vName) const
{
    return has(EPortType::OUTPUT, vName);
}

bool SPortDescriptor::hasInputOutput(const std::string& vName) const
{
    return has(EPortType::INPUT_OUTPUT, vName);
}

CPortSet::CPortSet(const SPortDescriptor& vDesc)
{
    for (const SPortDescriptor::SPortDescription& PortDesc : vDesc.PortDescSet)
    {
        switch (PortDesc.Type)
        {
            case SPortDescriptor::EPortType::INPUT:
            {
                __addInput(PortDesc.Name, PortDesc.Format);
                break;
            }
            case SPortDescriptor::EPortType::OUTPUT:
            {
                __addOutput(PortDesc.Name, PortDesc.Format);
                break;
            }
            case SPortDescriptor::EPortType::INPUT_OUTPUT:
            {
                __addInputOutput(PortDesc.Name, PortDesc.Format);
                break;
            }
            default:
            {
                _SHOULD_NOT_GO_HERE;
            }
        }
    }
}

bool CPortSet::isReady() const
{
    for (const auto& pPort : m_InputPortSet)
        if (!pPort->isReady()) return false;
    for (const auto& pPort : m_OutputPortSet)
        if (!pPort->isReady()) return false;
    return true;
}

void CPortSet::assertReady() const
{
    for (const auto& pPort : m_InputPortSet)
        pPort->assertReady();
    for (const auto& pPort : m_OutputPortSet)
        pPort->assertReady();
}

bool CPortSet::isInputReady() const
{
    for (const auto& pPort : m_InputPortSet)
        if (!pPort->isReady()) return false;
    return true;
}

void CPortSet::assertInputReady() const
{
    for (const auto& pPort : m_InputPortSet)
        if (!pPort->isReady())
            Common::throwError("input port " + pPort->getName() + " is not ready");
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

const SPortInfo& CPortSet::getInputFormat(const std::string& vName) const
{
    return getInputPort(vName)->getInfo();
}

const SPortInfo& CPortSet::getOutputFormat(const std::string& vName) const
{
    return getOutputPort(vName)->getInfo();
}

void CPortSet::setOutput(const std::string& vOutputName, VkImageView vImageView, VkFormat vFormat, VkExtent2D vExtent, VkImageLayout vLayout)
{
    SPortInfo InputInfo;
    InputInfo.Format = vFormat;
    InputInfo.Extent = vExtent;
    InputInfo.Num = 0; // dont care

    auto pPort = getOutputPort(vOutputName);
    const auto& TargetInfo = pPort->getInfo();
    _ASSERTE(InputInfo.isMatch(TargetInfo));

    CSourcePort::Ptr pSourcePort = std::dynamic_pointer_cast<CSourcePort>(pPort);

    // FIXME: what if different formats/extents are set? how to deal this?
    pSourcePort->setActualFormat(vFormat);
    pSourcePort->setActualExtent(vExtent);
    pSourcePort->setImage(vImageView);
}

void CPortSet::setOutput(const std::string& vOutputName, vk::CImage::Ptr vImage)
{
    VkFormat Format = vImage->getFormat();
    VkExtent2D Extent = vImage->getExtent();
    VkImageLayout Layout = vImage->getLayout();

    setOutput(vOutputName, *vImage, Format, Extent, Layout);
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

void CPortSet::__addInput(const std::string& vName, const SPortInfo& vInfo)
{
    _ASSERTE(!hasInput(vName));
    auto pPort = make<CRelayPort>(vName, vInfo, this);
    pPort->setInputLayout(__toLayout(vInfo.Usage));
    m_InputPortSet.emplace_back(pPort);
}

void CPortSet::__addOutput(const std::string& vName, const SPortInfo& vInfo)
{
    _ASSERTE(!hasOutput(vName));
    auto pPort = make<CSourcePort>(vName, vInfo, this);
    m_OutputPortSet.emplace_back(pPort);
}

void CPortSet::__addInputOutput(const std::string& vName, const SPortInfo& vInfo)
{
    _ASSERTE(!hasOutput(vName));
    _ASSERTE(!hasInput(vName));
    auto pPort = make<CRelayPort>(vName, vInfo, this);
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
