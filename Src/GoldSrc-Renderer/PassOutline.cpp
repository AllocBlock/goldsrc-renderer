#include "PassOutline.h"
#include "InterfaceGui.h"

#include <vector>
#include <ImageUtils.h>

void CRenderPassOutline::_initV()
{
    m_pMaskImage = make<vk::CImage>();
    ImageUtils::createImage2d(*m_pMaskImage, m_pDevice, m_ScreenExtent, VkFormat::VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    
    m_PassMaskDescriptor.addColorAttachment(m_pMaskImage, true);
    m_MaskPipeline.create(m_pDevice, m_PassMaskDescriptor, m_ScreenExtent);

    m_PassOutlineDescriptor.addColorAttachment(m_pPortSet->getOutputPort("Main"));
    m_EdgePipeline.create(m_pDevice, m_PassOutlineDescriptor, m_ScreenExtent);
    
    __createVertexBuffer();
}

sptr<CPortSet> CRenderPassOutline::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
    return make<CPortSet>(PortDesc);
}

void CRenderPassOutline::_updateV()
{
    _ASSERTE(m_pSceneInfo);
    sptr<CCamera> pCamera = m_pSceneInfo->pScene->getMainCamera();
    m_MaskPipeline.updateUniformBuffer(pCamera);
}

std::vector<VkCommandBuffer> CRenderPassOutline::_requestCommandBuffersV()
{
    sptr<CCommandBuffer> pCommandBuffer = _getCommandBuffer();

    _beginCommand(pCommandBuffer);

    m_pMaskImage->transitionLayout(pCommandBuffer, m_pMaskImage->getLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // 1. draw mask
    {
        _beginRendering(pCommandBuffer, m_PassMaskDescriptor.generateRendererInfo(m_ScreenExtent));
        m_MaskPipeline.recordCommand(pCommandBuffer);
        _endRendering();
    }

    // 2. get edge and blend to main
    m_pMaskImage->transitionLayout(pCommandBuffer, m_pMaskImage->getLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    {
        _beginRendering(pCommandBuffer, m_PassOutlineDescriptor.generateRendererInfo(m_ScreenExtent));

        m_EdgePipeline.setInputImage(*m_pMaskImage);

        if (m_pVertexBuffer->isValid())
        {
            pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
            m_EdgePipeline.bind(pCommandBuffer);
            pCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
        }
        _endRendering();
    }

    _endCommand();

    return { pCommandBuffer->get() };
}

void CRenderPassOutline::_destroyV()
{
    destroyAndClear(m_pMaskImage);
    m_MaskPipeline.destroy();
    m_EdgePipeline.destroy();
    destroyAndClear(m_pVertexBuffer);
}

void CRenderPassOutline::__createVertexBuffer()
{
    m_PointDataSet =
    {
        SFullScreenPointData{glm::vec2(-1.0f, -1.0f)},
        SFullScreenPointData{glm::vec2(-1.0f,  3.0f)},
        SFullScreenPointData{glm::vec2( 3.0f, -1.0f)},
    };
    
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SFullScreenPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}
