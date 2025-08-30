#include "PassPresent.h"

#include "RenderPassSingleFrameBuffer.h"

CRenderPassPresent::CRenderPassPresent(wptr<vk::CSwapchain> vpSwapchain)
{
    m_pSwapchain = vpSwapchain;
}

CPortSet::Ptr CRenderPassPresent::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addInput("Main", SPortInfo::createAnyOfUsage(EImageUsage::READ));
    return make<CPortSet>(PortDesc);
}

void CRenderPassPresent::__createCommandPoolAndBuffers()
{
    __destroyCommandPoolAndBuffers();
    m_Command.createPool(m_pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_DefaultCommandName, ECommandBufferLevel::PRIMARY);
}

void CRenderPassPresent::__destroyCommandPoolAndBuffers()
{
    m_Command.clear();
}

void CRenderPassPresent::__createFramebuffers(VkExtent2D vExtent)
{
    _ASSERTE(isValid());
    if (m_pSwapchain.expired()) throw "Swapchain is destroied";
    auto pSwapchain = m_pSwapchain.lock();

    m_pPortSet->assertReady();

    m_FramebufferSet.destroyAndClearAll();

    const auto& Images = pSwapchain->getImageViews();
    m_FramebufferSet.init(Images.size());
    for (uint32_t i = 0; i < Images.size(); ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            Images[i]
        };
        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
    }
}

void CRenderPassPresent::_bindVertexBuffer(CCommandBuffer::Ptr vCommandBuffer)
{
    _ASSERTE(m_pVertexBuffer->isValid());
    vCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
}

void CRenderPassPresent::__createVertexBuffer()
{
    __generateScene();

    if (!m_PointDataSet.empty())
    {
        m_pVertexBuffer = make<vk::CVertexBufferTyped<SFullScreenPointData>>();
        m_pVertexBuffer->create(m_pDevice, m_PointDataSet);
    }
}

void CRenderPassPresent::__generateScene()
{
    m_PointDataSet =
    {
        SFullScreenPointData{glm::vec2(-1.0f, -1.0f)},
        SFullScreenPointData{glm::vec2(-1.0f, 3.0f)},
        SFullScreenPointData{glm::vec2(3.0f, -1.0f)},
    };
}

void CRenderPassPresent::updateSwapchainImageIndex(uint32_t vImageIndex)
{
    m_CurrentSwapchainImageIndex = vImageIndex;
}

void CRenderPassPresent::_initV()
{
    __createCommandPoolAndBuffers();

    VkClearValue Value;
    Value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_ClearValueSet = { Value };
    __createFramebuffers(m_ScreenExtent);
    __createVertexBuffer();
        
    m_BlitPipeline.create(m_pDevice, get(), m_ScreenExtent);

    auto pInputPort = m_pPortSet->getInputPort("Main");
    m_BlitPipeline.setInputImage(pInputPort->getImageV());
}

void CRenderPassPresent::_destroyV()
{
    m_BlitPipeline.destroy();
    destroyAndClear(m_pVertexBuffer);
    m_FramebufferSet.destroyAndClearAll();
    __destroyCommandPoolAndBuffers();
}

CRenderPassDescriptor CRenderPassPresent::_getRenderPassDescV()
{
    if (m_pSwapchain.expired()) throw "Swapchain is destroied";
    auto pSwapchain = m_pSwapchain.lock();
    VkFormat Format = pSwapchain->getImageFormat();
    VkImageLayout InputLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout OutputLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    CRenderPassDescriptor Desc;
    Desc.addColorAttachment({ Format, InputLayout, OutputLayout, true, true });
    return Desc;
}

std::vector<VkCommandBuffer> CRenderPassPresent::_requestCommandBuffersV()
{
    _ASSERTE(m_FramebufferSet.isValid(m_CurrentSwapchainImageIndex));

    CCommandBuffer::Ptr pCommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName);

    _ASSERTE(m_FramebufferSet[m_CurrentSwapchainImageIndex]->getAttachmentNum() == m_ClearValueSet.size());
    _begin(pCommandBuffer, m_FramebufferSet[m_CurrentSwapchainImageIndex], m_ClearValueSet, false);

    m_BlitPipeline.bind(pCommandBuffer);
    _bindVertexBuffer(pCommandBuffer);
    pCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
    _end();
    return { pCommandBuffer->get() };
}