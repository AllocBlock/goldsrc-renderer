#include "PassOutline.h"
#include "InterfaceGui.h"
#include "RenderPassDescriptor.h"

#include <vector>

void CRenderPassOutline::_initV()
{
    m_pRerecord = make<CRerecordState>(m_ImageNum);
    m_pRerecord->addField("Primary");

    VkExtent2D RefExtent = { 0, 0 };
    bool Success = _dumpReferenceExtentV(RefExtent);
    _ASSERTE(Success);
    
    VkFormat Format = VkFormat::VK_FORMAT_R8_UNORM;
    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = static_cast<uint32_t>(RefExtent.width);
    ImageInfo.extent.height = static_cast<uint32_t>(RefExtent.height);
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = Format;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    m_MaskImage.create(m_pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);

    CRenderPassSingleFrameBuffer::_initV();

    m_MaskPipeline.create(m_pDevice, get(), RefExtent, m_ImageNum, 0);
    m_EdgePipeline.create(m_pDevice, get(), RefExtent, m_ImageNum, 1);
    
    __createVertexBuffer();

    m_pRerecord->requestRecordForAll();
}

void CRenderPassOutline::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
}

CRenderPassDescriptor CRenderPassOutline::_getRenderPassDescV()
{
    CRenderPassDescriptor Desc;

    SAttachementInfo MaskAttachementInfo;
    MaskAttachementInfo.Format = VkFormat::VK_FORMAT_R8_UNORM;
    MaskAttachementInfo.InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    MaskAttachementInfo.FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    MaskAttachementInfo.IsBegin = true;
    MaskAttachementInfo.IsEnd = false;
    Desc.addColorAttachment(MaskAttachementInfo);

    Desc.addColorAttachment(m_pPortSet->getOutputPort("Main"));

    SSubpassReferenceInfo SubPassInfo;

    Desc.addSubpass(SSubpassReferenceInfo().addColorRef(0).addDependentPass(VK_SUBPASS_EXTERNAL));
    Desc.addSubpass(SSubpassReferenceInfo().addColorRef(1).addInputRef(0).addDependentPass(0));
    return Desc;
}

void CRenderPassOutline::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pSceneInfo);
    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    m_MaskPipeline.updateUniformBuffer(vImageIndex, pCamera);
}

std::vector<VkCommandBuffer> CRenderPassOutline::_requestCommandBuffersV(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    if (m_pRerecord->consume("Primary"))
    {
        _beginWithFramebuffer(vImageIndex);
        // 1. draw mask
        m_MaskPipeline.recordCommand(pCommandBuffer, vImageIndex);

        pCommandBuffer->goNextPass();

        // 2. get edge and blend to main
        m_EdgePipeline.setInputImage(m_MaskImage, vImageIndex);

        if (m_pVertexBuffer->isValid())
        {
            pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
            m_EdgePipeline.bind(pCommandBuffer, vImageIndex);
            pCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
        }

        _endWithFramebuffer();
    }

    return { pCommandBuffer->get() };
}

void CRenderPassOutline::_destroyV()
{
    m_MaskImage.destroy();
    m_MaskPipeline.destroy();
    m_EdgePipeline.destroy();
    destroyAndClear(m_pVertexBuffer);

    CRenderPassSingleFrameBuffer::_destroyV();
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
