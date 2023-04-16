#pragma once
#include "RenderPass.h"

class IDynamicResource
{
public:
    virtual ~IDynamicResource() = default;

    bool isReady() const;
    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) = 0;

protected:
    bool m_IsReady = false;
};

class CDynamicTexture : public IDynamicResource
{
public:
    virtual VkImageView getImageViewV(uint32_t vIndex = 0) = 0;
};

class CDynamicTextureInputPort : public CDynamicTexture
{
public:
    CDynamicTextureInputPort(CPort::CPtr vInputPort);

    virtual VkImageView getImageViewV(uint32_t vIndex = 0) override;
    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    std::vector<VkImageView> __getCurrentImageViews() const;

    CPort::CPtr m_pInputPort = nullptr;
    std::vector<VkImageView> m_LastImageViews;
};


#include "Image.h"
using CreateImageCallback_t = std::function<void(VkExtent2D, vk::CPointerSet<vk::CImage>&)>;
class CDynamicTextureCreator : public CDynamicTexture
{
public:
    void init(VkExtent2D vInitExtent, bool vKeepScreenSize, CreateImageCallback_t vOnCreateImageFunc);

    virtual VkImageView getImageViewV(uint32_t vIndex = 0) override;
    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) override;

    bool updateExtent(VkExtent2D vNewExtent, bool vForceUpdate = false);
    void destroy();

private:
    bool __isCreatable();
    void __createOrRecreateImage();

    bool m_IsInitted = false;
    VkExtent2D m_Extent = VkExtent2D{ 0, 0 };
    bool m_KeepScreenSize = false;
    CreateImageCallback_t m_CreateImageFunc;
    vk::CPointerSet<vk::CImage> m_ImageSet;
};

template <typename Pipeline_t>
using CreatePipelineCallback_t = std::function<void(Pipeline_t&)>;

template <typename Pipeline_t>
class CDynamicPipeline : public IDynamicResource
{
public:
    void init(vk::CDevice::CPtr vDevice, std::weak_ptr<const vk::IRenderPass> vPass, VkExtent2D vInitExtent, bool vKeepScreenSize, uint32_t vImageNum, CreatePipelineCallback_t<Pipeline_t> vCreateCallback = nullptr, uint32_t vSubpass = 0)
    {
        _ASSERTE(!m_IsInitted);
        _ASSERTE(!vPass.expired());
        _ASSERTE(vDevice);
        m_Extent = vInitExtent;
        m_KeepScreenSize = vKeepScreenSize;
        m_ImageNum = vImageNum;
        m_pPass = vPass;
        m_pDevice = vDevice;
        m_pCreateCallback = vCreateCallback;
        m_Subpass = vSubpass;
        m_IsInitted = true;
    }

    virtual bool updateV(const vk::SPassUpdateState& vUpdateState) override
    {
        _ASSERTE(m_IsInitted);
        bool NeedRecreate = vUpdateState.RenderpassUpdated; // renderpass updated

        // screen extent updated
        if (m_KeepScreenSize && vUpdateState.ScreenExtent.IsUpdated && vUpdateState.ScreenExtent.Value != m_Extent)
        {
            NeedRecreate = true;
            m_Extent = vUpdateState.ScreenExtent.Value;
        }

        if (vUpdateState.ImageNum.IsUpdated && vUpdateState.ImageNum.Value != m_ImageNum)
        {
            m_ImageNum = vUpdateState.ImageNum.Value;
            if (m_Pipeline.isValid())
            {
                // only need to update image num
                m_Pipeline.setImageNum(m_ImageNum);
                return true;
            }
            NeedRecreate = true;
        }

        if (NeedRecreate)
        {
            recreate();
            return true;
        }
        return false;
    }

    Pipeline_t& get()
    {
        _ASSERTE(m_IsInitted);
        _ASSERTE(isReady());
        return m_Pipeline;
    }

    bool updateExtent(VkExtent2D vNewExtent)
    {
        _ASSERTE(m_IsInitted);
        if (m_Extent != vNewExtent)
        {
            m_Extent = vNewExtent;
            recreate();
            return true;
        }
        return false;
    }

    void recreate()
    {
        _ASSERTE(m_IsInitted);
        if (m_Extent.width == 0 || m_Extent.height == 0 || m_ImageNum == 0)
        {
            m_Pipeline.destroy();
        }
        else
        {
            auto pPass = m_pPass.lock();
            if (pPass->isValid())
            {
                m_Pipeline.create(m_pDevice, pPass->get(), m_Extent, m_Subpass);
                m_Pipeline.setImageNum(m_ImageNum);
                if (m_pCreateCallback)
                    m_pCreateCallback(m_Pipeline);
            }
        }
        m_IsReady = m_Pipeline.isValid();
    }

    void destroy()
    {
        m_Pipeline.destroy();
    }

protected:
    bool m_IsInitted = false;
    Pipeline_t m_Pipeline;

    VkExtent2D m_Extent = VkExtent2D{ 0, 0 };
    bool m_KeepScreenSize = false;
    uint32_t m_ImageNum = 0;
    uint32_t m_Subpass = 0;
    std::weak_ptr<const vk::IRenderPass> m_pPass;
    vk::CDevice::CPtr m_pDevice = nullptr;
    CreatePipelineCallback_t<Pipeline_t> m_pCreateCallback = nullptr;
};