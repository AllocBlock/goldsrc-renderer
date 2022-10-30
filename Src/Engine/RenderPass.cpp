#include "RenderPass.h"

using namespace vk;

std::vector<VkClearValue> __createDefaultClearValueColor()
{
    VkClearValue Value;
    Value.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    return { Value };
}

std::vector<VkClearValue> __createDefaultClearValueColorDepth()
{
    std::vector<VkClearValue> ValueSet(2);
    ValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ValueSet[1].depthStencil = { 1.0f, 0 };

    return ValueSet;
}

const std::vector<VkClearValue>& IRenderPass::DefaultClearValueColor = __createDefaultClearValueColor();
const std::vector<VkClearValue>& IRenderPass::DefaultClearValueColorDepth = __createDefaultClearValueColorDepth();

IRenderPass::IRenderPass()
{
}

void IRenderPass::init(vk::CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo)
{
    m_pDevice = vDevice;
    m_pAppInfo = vAppInfo;

    auto PortDesc = _getPortDescV();
    m_pPortSet = make<CPortSet>(PortDesc, this);

    // FIXME: init first or create pass/command first
    _initV();

    __hookEvents();
    __updateImageNum(m_pAppInfo->getImageNum());
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
    __destroyCommandPoolAndBuffers();
    __unhookEvents();
    m_pPortSet->unlinkAll();
}

void IRenderPass::begin(VkCommandBuffer vCommandBuffer, CFrameBuffer::CPtr vFrameBuffer, const std::vector<VkClearValue>& vClearValues)
{
    _ASSERTE(!m_Begined);
    _ASSERTE(isValid());

    // only need one command one pass, so begin/end command buffer at same time with renderpass
    __beginCommand(vCommandBuffer);

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = get();
    RenderPassBeginInfo.framebuffer = *vFrameBuffer;
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = vFrameBuffer->getExtent();
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(vClearValues.size());
    RenderPassBeginInfo.pClearValues = vClearValues.data();

    vkCmdBeginRenderPass(vCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    m_CurrentCommandBuffer = vCommandBuffer;
    m_Begined = true;
}

void IRenderPass::end()
{
    _ASSERTE(m_Begined);
    vkCmdEndRenderPass(m_CurrentCommandBuffer);
    __endCommand(m_CurrentCommandBuffer);
    m_CurrentCommandBuffer = VK_NULL_HANDLE;
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

void IRenderPass::__createCommandPoolAndBuffers(uint32_t vImageNum)
{
    __destroyCommandPoolAndBuffers();
    m_Command.createPool(m_pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_DefaultCommandName, static_cast<uint32_t>(vImageNum), ECommandBufferLevel::PRIMARY);
}

void IRenderPass::__destroyCommandPoolAndBuffers()
{
    m_Command.clear();
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

void IRenderPass::__beginCommand(VkCommandBuffer vCommandBuffer)
{
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // TODO: more flag options?

    vk::checkError(vkBeginCommandBuffer(vCommandBuffer, &CommandBufferBeginInfo));
}


void IRenderPass::__endCommand(VkCommandBuffer vCommandBuffer)
{
    vk::checkError(vkEndCommandBuffer(vCommandBuffer));
}

void IRenderPass::__updateImageNum(uint32_t vImageNum)
{
    __createCommandPoolAndBuffers(vImageNum);
    __triggerImageNumUpdate(vImageNum);
}

void IRenderPass::__hookEvents()
{
    m_ImageNumUpdateHookId = m_pAppInfo->hookImageNumUpdate([this](uint32_t vImageNum) 
        { __updateImageNum(vImageNum); }
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
