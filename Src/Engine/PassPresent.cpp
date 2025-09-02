#include "PassPresent.h"

#include "RenderPass.h"

CRenderPassPresent::CRenderPassPresent(wptr<vk::CSwapchain> vpSwapchain)
{
    m_pSwapchain = vpSwapchain;
}

void CRenderPassPresent::updateSwapchainImageIndex(uint32_t vImageIndex)
{
    m_CurrentSwapchainImageIndex = vImageIndex;
}

CPortSet::Ptr CRenderPassPresent::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addInput("Main", SPortInfo::createAnyOfUsage(EImageUsage::READ));
    return make<CPortSet>(PortDesc);
}

void CRenderPassPresent::_initV()
{
    if (m_pSwapchain.expired()) throw "Swapchain is destroyed";
    auto pSwapchain = m_pSwapchain.lock();

    m_pPortSet->assertReady();
    const auto& Images = pSwapchain->getImageViews();
    m_RenderInfoDescriptors.resize(Images.size());
    for (uint32_t i = 0; i < Images.size(); ++i)
    {
        auto& builder = m_RenderInfoDescriptors[i];
        builder.clear();
        builder.addColorAttachment(Images[i], pSwapchain->getImageFormat(), false);
    }

    __createCommandPoolAndBuffers();
    __createVertexBuffer();

    m_BlitPipeline.create(m_pDevice, m_RenderInfoDescriptors[0], m_ScreenExtent); // no renderpass

    auto pInputPort = m_pPortSet->getInputPort("Main");
    m_BlitPipeline.setInputImage(*pInputPort->getImageV());
}

void CRenderPassPresent::_destroyV()
{
    m_BlitPipeline.destroy();
    destroyAndClear(m_pVertexBuffer);
    __destroyCommandPoolAndBuffers();
}

std::vector<VkCommandBuffer> CRenderPassPresent::_requestCommandBuffersV()
{
    if (m_pSwapchain.expired()) throw "Swapchain is destroyed";
    auto pSwapchain = m_pSwapchain.lock();
    auto pImage = pSwapchain->getImages()[m_CurrentSwapchainImageIndex];

    CCommandBuffer::Ptr pCommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName);

    _beginCommand(pCommandBuffer);
    pImage->transitionLayout(pCommandBuffer, pImage->getLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    _beginRendering(pCommandBuffer, m_RenderInfoDescriptors[m_CurrentSwapchainImageIndex].generateRendererInfo(m_ScreenExtent, false));

    m_BlitPipeline.bind(pCommandBuffer);
    _ASSERTE(m_pVertexBuffer->isValid());
    pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
    pCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
    _endRendering();
    pImage->transitionLayout(pCommandBuffer, pImage->getLayout(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    _endCommand();
    return { pCommandBuffer->get() };
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