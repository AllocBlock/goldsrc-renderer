#pragma once
#include "RenderPass.h"

class CDynamicResourceManager
{
};

class CDynamicResource
{
public:
    bool isReady() const { return m_IsReady; }
    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) = 0;

protected:
    bool m_IsReady = false;
};

class CDynamicTexture : public CDynamicResource
{
public:
    virtual VkImageView getImageViewV() = 0;
};

class CDynamicTextureInputPort : public CDynamicTexture
{
public:
    CDynamicTextureInputPort(CPort::CPtr vInputPort)
    {
        m_pInputPort = vInputPort;
        m_LastImageView = __getCurrentImageView();
    }

    virtual VkImageView getImageViewV()
    {
        _ASSERTE(isReady());
        return m_LastImageView.value();
    }

    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) override
    {
        if (vUpdateState.InputImageUpdated)
        {
            std::optional<VkImageView> currentImageView = __getCurrentImageView();
            if (currentImageView != m_LastImageView)
            {
                m_LastImageView = currentImageView;
                m_IsReady = (currentImageView != std::nullopt);
            }
        }
        return false;
    }

private:
    std::optional<VkImageView> __getCurrentImageView() const
    {
        if (!m_pInputPort->isImageReadyV()) return std::nullopt;
        return m_pInputPort->getImageV();
    }

    CPort::CPtr m_pInputPort = nullptr;
    std::optional<VkImageView> m_LastImageView = std::nullopt;
};


#include "Image.h"
class CDynamicTextureCreator : public CDynamicTexture
{
public:
    void init(VkExtent2D vInitExtent, bool vKeepScreenSize, std::function<std::unique_ptr<vk::CImage>(VkExtent2D)> vOnCreateImageFunc)
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

    virtual VkImageView getImageViewV()
    {
        _ASSERTE(m_IsInitted);
        _ASSERTE(isReady());
        _ASSERTE(m_pImage != nullptr);
        return m_pImage->get();
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
        _ASSERTE(m_IsInitted);
        if (m_pImage)
            m_pImage->destroy();
    }

private:
    bool __isCreatable()
    {
        return m_Extent.width > 0 && m_Extent.height > 0;
    }

    void __createOrRecreateImage()
    {
        if (m_pImage)
            destroy();
        m_pImage = m_CreateImageFunc(m_Extent);
    }

    bool m_IsInitted = false;
    VkExtent2D m_Extent = VkExtent2D{ 0, 0 };
    bool m_KeepScreenSize = false;
    std::function<std::unique_ptr<vk::CImage>(VkExtent2D)> m_CreateImageFunc;
    std::unique_ptr<vk::CImage> m_pImage = nullptr;
};