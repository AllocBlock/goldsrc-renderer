#pragma once
#include "RenderPass.h"

class CRenderPassSingle : public vk::IRenderPass
{
protected:
    virtual void _destroyV() override
    {
        m_FramebufferSet.destroyAndClearAll();
    }

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override
    {
        VkExtent2D Extent;
        if (!_dumpReferenceExtentV(Extent)) return;

        if (isValid() && (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated))
        {
            __createFramebuffers(Extent);
        }
    }

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) = 0;
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) = 0;
    
    VkCommandBuffer _beginWithFramebuffer(uint32_t vImageIndex)
    {
        _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

        VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);
        
        _begin(CommandBuffer, m_FramebufferSet[vImageIndex], m_ClearValueSet);
        return CommandBuffer;
    }

    void _endWithFramebuffer()
    {
        _end();
    }

private:
    using vk::IRenderPass::_begin;
    using vk::IRenderPass::_end;
    
    void __createFramebuffers(VkExtent2D vExtent)
    {
        if (!isValid()) return;

        m_FramebufferSet.destroyAndClearAll();

        size_t ImageNum = m_pAppInfo->getImageNum();
        m_FramebufferSet.init(ImageNum);
        for (size_t i = 0; i < ImageNum; ++i)
        {
            std::vector<VkImageView> AttachmentSet = _getAttachmentsV(i);
            m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
        }
    }
    
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    std::vector<VkClearValue> m_ClearValueSet;
};