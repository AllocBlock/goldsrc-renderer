#include "RenderPass.h"

using namespace vk;

IRenderPass::IRenderPass()
{
}

void IRenderPass::init(vk::CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo)
{
    m_pDevice = vDevice;
    m_pAppInfo = vAppInfo;

    m_pPortSet = _createPortSetV();

    // FIXME: init first or create pass/command first
    _initV();

    __hookEvents();
    __triggerImageNumUpdate(m_pAppInfo->getImageNum());
    __createRenderpass();
}

void IRenderPass::update(uint32_t vImageIndex)
{
    _ASSERTE(isValid());
    _updateV(vImageIndex);
}

std::vector<VkCommandBuffer> IRenderPass::requestCommandBuffers(uint32_t vImageIndex)
{
    _ASSERTE(isValid());
    return _requestCommandBuffersV(vImageIndex);
}

void IRenderPass::destroy()
{
    _destroyV();
    __destroyRenderpass();
    __unhookEvents();
    m_pPortSet->unlinkAll();
}

void IRenderPass::_begin(CCommandBuffer::Ptr vCommandBuffer, CFrameBuffer::CPtr vFrameBuffer, const std::vector<VkClearValue>& vClearValues, bool vHasSecondary)
{
    _ASSERTE(!m_Begined);
    _ASSERTE(isValid());
    if (m_pCurrentCommandBuffer != nullptr)
        throw std::runtime_error("Already begun with another command buffer");

    // only need one command one pass, so begin/end command buffer at same time with renderpass
    vCommandBuffer->begin();

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = get();
    RenderPassBeginInfo.framebuffer = *vFrameBuffer;
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = vFrameBuffer->getExtent();
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(vClearValues.size());
    RenderPassBeginInfo.pClearValues = vClearValues.data();

    vCommandBuffer->beginRenderPass(RenderPassBeginInfo, vHasSecondary);

    m_pCurrentCommandBuffer = vCommandBuffer;
    m_Begined = true;
}

void IRenderPass::_end()
{
    _ASSERTE(m_Begined);
    m_pCurrentCommandBuffer->endRenderPass();
    m_pCurrentCommandBuffer->end();
    m_pCurrentCommandBuffer = nullptr;
    m_Begined = false;
}

bool IRenderPass::_dumpInputPortExtent(std::string vName, VkExtent2D& voExtent)
{
    bool HasExtent = false;
    auto pRefPort = m_pPortSet->getInputPort(vName);
    if (pRefPort->hasActualExtentV())
    {
        HasExtent = true;
        voExtent = pRefPort->getActualExtentV();
    }
    return HasExtent;
}

void IRenderPass::__createRenderpass()
{
    CRenderPassDescriptor OldDesc = m_CurPassDesc;
    m_CurPassDesc = _getRenderPassDescV();
    if (OldDesc == m_CurPassDesc) return; // same, no change
    if (!OldDesc.isValid() && !m_CurPassDesc.isValid()) return; // all invalid, no change

    __destroyRenderpass();
    if (m_CurPassDesc.isValid())
    {
        auto Info = m_CurPassDesc.generateInfo();
        vk::checkError(vkCreateRenderPass(*m_pDevice, &Info, nullptr, _getPtr()));
        m_CurPassDesc.clearStage(); // free stage data to save memory

#ifdef _DEBUG
        static int Count = 0;
        std::cout << "create renderpass [" << Count << "] = 0x" << std::setbase(16) << uint64_t(get()) << std::setbase(10) << std::endl;
        Count++;
#endif
    }

    __triggerRenderpassUpdate();
}

void IRenderPass::__destroyRenderpass()
{
    if (isValid())
    {
        vkDestroyRenderPass(*m_pDevice, get(), nullptr);
        _setNull();
    }
}

void IRenderPass::__hookEvents()
{
    m_ImageNumUpdateHookId = m_pAppInfo->hookImageNumUpdate([this](uint32_t vImageNum) 
        { __triggerImageNumUpdate(vImageNum); }
    );
    m_ScreenExtentUpdateHookId = m_pAppInfo->hookScreenExtentUpdate([this](VkExtent2D vExtent)      
        { __triggerScreenExtentUpdate(vExtent); }
    );

    m_InputImageUpdateHookId = m_pPortSet->hookInputImageUpdate([this]()
        { __triggerInputImageUpdate(); }
    );

    m_LinkUpdateHookId = m_pPortSet->hookLinkUpdate([this](EventId_t, ILinkEvent::CPtr)
        { __createRenderpass(); }
    );
}

void IRenderPass::__unhookEvents()
{
    if (m_ImageNumUpdateHookId) m_pAppInfo->unhookImageNumUpdate(m_ImageNumUpdateHookId);
    if (m_ScreenExtentUpdateHookId) m_pAppInfo->unhookScreenExtentUpdate(m_ScreenExtentUpdateHookId);
    if (m_InputImageUpdateHookId) m_pPortSet->unhookInputImageUpdate(m_InputImageUpdateHookId);
    if (m_LinkUpdateHookId) m_pPortSet->unhookLinkUpdate(m_LinkUpdateHookId);
}

void IRenderPass::__triggerImageNumUpdate(uint32_t vImageNum)
{
    SPassUpdateState State(m_pAppInfo->getImageNum(), m_pAppInfo->getScreenExtent());
    State.ImageNum = SPassUpdateAttribute<uint32_t>::create(vImageNum, true);
    _onUpdateV(State);
}

void IRenderPass::__triggerScreenExtentUpdate(VkExtent2D vExtent)
{
    SPassUpdateState State(m_pAppInfo->getImageNum(), m_pAppInfo->getScreenExtent());
    State.ScreenExtent = SPassUpdateAttribute<VkExtent2D>::create(vExtent, true);
    _onUpdateV(State);
}

void IRenderPass::__triggerInputImageUpdate()
{
    SPassUpdateState State(m_pAppInfo->getImageNum(), m_pAppInfo->getScreenExtent());
    State.InputImageUpdated = true;
    _onUpdateV(State);
}

void IRenderPass::__triggerRenderpassUpdate()
{
    SPassUpdateState State(m_pAppInfo->getImageNum(), m_pAppInfo->getScreenExtent());
    State.RenderpassUpdated = true;
    _onUpdateV(State);
}
