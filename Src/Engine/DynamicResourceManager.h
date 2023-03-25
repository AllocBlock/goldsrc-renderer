#pragma once
#include "RenderPass.h"

class CDynamicResourceManager
{
};

class CDynamicResource
{
public:
    virtual ~CDynamicResource() = default;

    bool isReady() const { return m_IsReady; }
    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) = 0;

protected:
    bool m_IsReady = false;
};

class CDynamicTexture : public CDynamicResource
{
public:
    virtual VkImageView getImageViewV(uint32_t vIndex = 0) = 0;
};

class CDynamicTextureInputPort : public CDynamicTexture
{
public:
    CDynamicTextureInputPort(CPort::CPtr vInputPort)
    {
        m_pInputPort = vInputPort;
        m_LastImageViews = __getCurrentImageViews();
    }

    virtual VkImageView getImageViewV(uint32_t vIndex = 0) override
    {
        _ASSERTE(isReady());
        _ASSERTE(vIndex < m_LastImageViews.size());
        return m_LastImageViews[vIndex];
    }

    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) override
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

private:
    std::vector<VkImageView> __getCurrentImageViews() const
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

    CPort::CPtr m_pInputPort = nullptr;
    std::vector<VkImageView> m_LastImageViews;
};


#include "Image.h"
using CreateImageCallback_t = std::function<void(VkExtent2D, vk::CPointerSet<vk::CImage>&)>;
class CDynamicTextureCreator : public CDynamicTexture
{
public:
    void init(VkExtent2D vInitExtent, bool vKeepScreenSize, CreateImageCallback_t vOnCreateImageFunc)
    {
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

    virtual VkImageView getImageViewV(uint32_t vIndex = 0) override
    {
        _ASSERTE(m_IsInitted);
        _ASSERTE(isReady());
        _ASSERTE(m_ImageSet.isValid(vIndex));
        return m_ImageSet[vIndex]->get();
    }

    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) override
    {
        _ASSERTE(m_IsInitted);
        if (m_KeepScreenSize && vUpdateState.ScreenExtent.IsUpdated)
        {
            return updateExtent(vUpdateState.ScreenExtent.Value);
        }
        return false;
    }

    bool updateExtent(VkExtent2D vNewExtent)
    {
        _ASSERTE(m_IsInitted);
        if (m_Extent != vNewExtent)
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

    void destroy()
    {
        m_ImageSet.destroyAndClearAll();
    }

private:
    bool __isCreatable()
    {
        return m_Extent.width > 0 && m_Extent.height > 0;
    }

    void __createOrRecreateImage()
    {
        destroy();
        m_CreateImageFunc(m_Extent, m_ImageSet);
    }

    bool m_IsInitted = false;
    VkExtent2D m_Extent = VkExtent2D{ 0, 0 };
    bool m_KeepScreenSize = false;
    CreateImageCallback_t m_CreateImageFunc;
    vk::CPointerSet<vk::CImage> m_ImageSet;
};