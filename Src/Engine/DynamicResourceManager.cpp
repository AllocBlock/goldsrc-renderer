#include "DynamicResourceManager.h"

bool IDynamicResource::isReady() const
{ return m_IsReady; }

CDynamicTextureInputPort::CDynamicTextureInputPort(CPort::CPtr vInputPort)
{
    m_pInputPort = vInputPort;
    m_LastImageViews = __getCurrentImageViews();
}

VkImageView CDynamicTextureInputPort::getImageViewV(uint32_t vIndex)
{
    _ASSERTE(isReady());
    _ASSERTE(vIndex < m_LastImageViews.size());
    return m_LastImageViews[vIndex];
}

bool CDynamicTextureInputPort::updateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.InputImageUpdated)
    {
        auto currentImageView = __getCurrentImageViews();
        if (!Common::isVectorEqual(currentImageView, m_LastImageViews))
        {
            m_LastImageViews = currentImageView;
            m_IsReady = (currentImageView.size() > 0);
        }
    }
    return false;
}

std::vector<VkImageView> CDynamicTextureInputPort::__getCurrentImageViews() const
{
    if (!m_pInputPort->isImageReadyV()) return {};
    size_t ImageNum = m_pInputPort->getImageNumV();
    if (ImageNum == 0) return {};
    std::vector<VkImageView> Views;
    for (size_t i = 0; i < ImageNum; ++i)
    {
        Views.push_back(m_pInputPort->getImageV(i));
    }
    return Views;
}

void CDynamicTextureCreator::init(VkExtent2D vInitExtent, bool vKeepScreenSize,
    CreateImageCallback_t vOnCreateImageFunc)
{
    _ASSERTE(!m_IsInitted);
    m_Extent = vInitExtent;
    m_KeepScreenSize = vKeepScreenSize;
    m_CreateImageFunc = vOnCreateImageFunc;
    _ASSERTE(vOnCreateImageFunc);
    if (__isCreatable())
    {
        __createOrRecreateImage();
    }
    m_IsInitted = true;
}

VkImageView CDynamicTextureCreator::getImageViewV(uint32_t vIndex)
{
    _ASSERTE(m_IsInitted);
    _ASSERTE(isReady());
    _ASSERTE(m_ImageSet.isValid(vIndex));
    return m_ImageSet[vIndex]->get();
}

bool CDynamicTextureCreator::updateV(const vk::SPassUpdateState& vUpdateState)
{
    _ASSERTE(m_IsInitted);
    if (m_KeepScreenSize && vUpdateState.ScreenExtent.IsUpdated)
    {
        return updateExtent(vUpdateState.ScreenExtent.Value);
    }
    return false;
}

bool CDynamicTextureCreator::updateExtent(VkExtent2D vNewExtent, bool vForceUpdate)
{
    _ASSERTE(m_IsInitted);
    if (vForceUpdate || m_Extent != vNewExtent)
    {
        m_Extent = vNewExtent;
        if (__isCreatable())
        {
            __createOrRecreateImage();
            return true;
        }
    }
    return false;
}

void CDynamicTextureCreator::destroy()
{
    m_ImageSet.destroyAndClearAll();
}

bool CDynamicTextureCreator::__isCreatable()
{
    return m_Extent.width > 0 && m_Extent.height > 0;
}

void CDynamicTextureCreator::__createOrRecreateImage()
{
    destroy();
    m_CreateImageFunc(m_Extent, m_ImageSet);
    _ASSERTE(!m_ImageSet.empty() && m_ImageSet.isAllValid());
    m_IsReady = true;
}
