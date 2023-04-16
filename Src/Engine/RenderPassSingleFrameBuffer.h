#pragma once
#include "RenderPass.h"

// render pass with single frame buffer
class CRenderPassSingleFrameBuffer : public vk::IRenderPass
{
protected:
    static const std::vector<VkClearValue>& DefaultClearValueColor;
    static const std::vector<VkClearValue>& DefaultClearValueColorDepth;

    virtual CPortSet::Ptr _createPortSetV() final;

    /*
     * _initPortDescV:
     * triggers only once
     * setup port info for PortSet
     */
    virtual void _initPortDescV(SPortDescriptor& vioDesc) = 0;

    // IMPORTANT: call this original _initV if override
    virtual void _initV() override;
    // IMPORTANT: call this original _destroyV if override
    virtual void _destroyV() override;
    // IMPORTANT: call this original _onUpdateV if override
    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) = 0;
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) = 0;
    virtual std::vector<VkClearValue> _getClearValuesV() = 0;
    virtual std::vector<std::string> _getExtraCommandBufferNamesV() const;;

    CCommandBuffer::Ptr _getCommandBuffer(uint32_t vImageIndex);
    void _beginWithFramebuffer(uint32_t vImageIndex, bool vHasSecondary = false);
    void _endWithFramebuffer();
    void _beginSecondary(CCommandBuffer::Ptr vCommandBuffer, uint32_t vImageIndex);

    CCommand m_Command = CCommand();

private:
    // hide original begin/end, use _beginWithFramebuffer/_endWithFramebuffer instead
    using vk::IRenderPass::_begin;
    using vk::IRenderPass::_end;
    
    void __createCommandPoolAndBuffers(uint32_t vImageNum);
    void __destroyCommandPoolAndBuffers();
    void __createFramebuffers(VkExtent2D vExtent);

    const std::string m_DefaultCommandName = "Default";
    
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    std::vector<VkClearValue> m_ClearValueSet;
};