#pragma once
#include "RenderPass.h"

// render pass with single frame buffer
class CRenderPassSingle : public vk::IRenderPass
{
protected:
    static const std::vector<VkClearValue>& DefaultClearValueColor;
    static const std::vector<VkClearValue>& DefaultClearValueColorDepth;

    virtual CPortSet::Ptr _createPortSetV() final
    {
        SPortDescriptor PortDesc;
        _initPortDescV(PortDesc);
        return make<CPortSet>(PortDesc, this);
    }

    /*
     * _initPortDescV:
     * triggers only once
     * setup port info for PortSet
     */
    virtual void _initPortDescV(SPortDescriptor& vioDesc) = 0;
    
    virtual void _initV() override
    {
        m_ClearValueSet = _getClearValuesV();
    }

    virtual void _destroyV() override
    {
        m_FramebufferSet.destroyAndClearAll();
        __destroyCommandPoolAndBuffers();
    }

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override
    {
        if (vUpdateState.ImageNum.IsUpdated)
        {
            __createCommandPoolAndBuffers(vUpdateState.ImageNum.Value);
        }

        if (isValid() && m_pPortSet->isImageReady() && 
            (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated || vUpdateState.InputImageUpdated || vUpdateState.ScreenExtent.IsUpdated))
        {
            VkExtent2D Extent;
            if (!_dumpReferenceExtentV(Extent)) return;
            if (Extent != vk::ZeroExtent)
                __createFramebuffers(Extent);
        }
    }

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) = 0;
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) = 0;
    virtual std::vector<VkClearValue> _getClearValuesV() = 0;

    VkCommandBuffer _getCommandBuffer(uint32_t vImageIndex)
    {
        return m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);
    }
    
    void _beginWithFramebuffer(uint32_t vImageIndex)
    {
        _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

        VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

        _ASSERTE(m_FramebufferSet[vImageIndex]->getAttachmentNum() == m_ClearValueSet.size());
        _begin(CommandBuffer, m_FramebufferSet[vImageIndex], m_ClearValueSet);
    }

    void _endWithFramebuffer()
    {
        _end();
    }

    CCommand m_Command = CCommand();

private:
    using vk::IRenderPass::_begin;
    using vk::IRenderPass::_end;
    
    void __createCommandPoolAndBuffers(uint32_t vImageNum)
    {
        __destroyCommandPoolAndBuffers();
        if (vImageNum == 0) return;
        m_Command.createPool(m_pDevice, ECommandType::RESETTABLE);
        m_Command.createBuffers(m_DefaultCommandName, static_cast<uint32_t>(vImageNum), ECommandBufferLevel::PRIMARY);
    }

    void __destroyCommandPoolAndBuffers()
    {
        m_Command.clear();
    }
    
    void __createFramebuffers(VkExtent2D vExtent)
    {
        if (!isValid()) return;
        if (!m_pPortSet->isImageReady()) return;

        m_FramebufferSet.destroyAndClearAll();

        size_t ImageNum = m_pAppInfo->getImageNum();
        m_FramebufferSet.init(ImageNum);
        for (size_t i = 0; i < ImageNum; ++i)
        {
            std::vector<VkImageView> AttachmentSet = _getAttachmentsV(i);
            m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
        }
    }

    const std::string m_DefaultCommandName = "Default";
    
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    std::vector<VkClearValue> m_ClearValueSet;
};