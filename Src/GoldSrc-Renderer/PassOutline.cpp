#include "PassOutline.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"

#include <vector>

void CRenderPassOutline::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();
    
    m_pRerecord = make<CRerecordState>(m_pAppInfo);
    m_pRerecord->addField("Primary");

    VkExtent2D RefExtent = { 0, 0 };
    _dumpReferenceExtentV(RefExtent);

    m_MaskImageCreator.init(RefExtent, false,
        [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
        {
            VkFormat Format = VkFormat::VK_FORMAT_R8_UNORM;

            vImageSet.init(m_pAppInfo->getImageNum());
            for (size_t i = 0; i < m_pAppInfo->getImageNum(); ++i)
            {
                VkImageCreateInfo ImageInfo = {};
                ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                ImageInfo.imageType = VK_IMAGE_TYPE_2D;
                ImageInfo.extent.width = static_cast<uint32_t>(vExtent.width);
                ImageInfo.extent.height = static_cast<uint32_t>(vExtent.height);
                ImageInfo.extent.depth = 1;
                ImageInfo.mipLevels = 1;
                ImageInfo.arrayLayers = 1;
                ImageInfo.format = Format;
                ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                vk::SImageViewInfo ViewInfo;
                ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

                vImageSet[i]->create(m_pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
            }
        }
    );

    m_MaskPipelineCreator.init(m_pDevice, weak_from_this(), RefExtent, false, m_pAppInfo->getImageNum(),
        [this](IPipeline& vPipeline) { m_pRerecord->requestRecordForAll(); }, 0
    );

    m_EdgePipelineCreator.init(m_pDevice, weak_from_this(), RefExtent, false, m_pAppInfo->getImageNum(), nullptr, 1);
    
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

    Desc.addSubpass({ 0 }, false, { VK_SUBPASS_EXTERNAL });
    Desc.addSubpass({ 1 }, false, { 0 });
    return Desc;
}

void CRenderPassOutline::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    m_MaskImageCreator.updateV(vUpdateState);
    m_MaskPipelineCreator.updateV(vUpdateState);
    m_EdgePipelineCreator.updateV(vUpdateState);

    VkExtent2D RefExtent = { 0, 0 };
    if (_dumpReferenceExtentV(RefExtent))
    {
        m_MaskImageCreator.updateExtent(RefExtent);
        m_MaskPipelineCreator.updateExtent(RefExtent); m_EdgePipelineCreator.updateExtent(RefExtent);
    }

    CRenderPassSingleFrameBuffer::_onUpdateV(vUpdateState);
}

void CRenderPassOutline::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pSceneInfo);
    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    m_MaskPipelineCreator.get().updateUniformBuffer(vImageIndex, pCamera);
}

std::vector<VkCommandBuffer> CRenderPassOutline::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_EdgePipelineCreator.isReady());

    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    if (m_pRerecord->consume("Primary"))
    {
        _beginWithFramebuffer(vImageIndex);
        // 1. draw mask
        m_MaskPipelineCreator.get().recordCommand(pCommandBuffer, vImageIndex);

        pCommandBuffer->goNextPass();

        // 2. get edge and blend to main
        auto& Pipeline = m_EdgePipelineCreator.get();
        Pipeline.setInputImage(m_MaskImageCreator.getImageViewV(0), vImageIndex);

        if (m_pVertexBuffer->isValid())
        {
            pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
            Pipeline.bind(pCommandBuffer, vImageIndex);
            pCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
        }

        _endWithFramebuffer();
    }

    return { pCommandBuffer->get() };
}

void CRenderPassOutline::_destroyV()
{
    m_MaskImageCreator.destroy();
    m_MaskPipelineCreator.destroy();
    m_EdgePipelineCreator.destroy();
    destroyAndClear(m_pVertexBuffer);

    CRenderPassSingleFrameBuffer::_destroyV();
}

void CRenderPassOutline::__createVertexBuffer()
{
    m_PointDataSet =
    {
        CPipelineEdge::SPointData{glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)},
        CPipelineEdge::SPointData{glm::vec2(-1.0f,  3.0f), glm::vec2(0.0f, 2.0f)},
        CPipelineEdge::SPointData{glm::vec2( 3.0f, -1.0f), glm::vec2(2.0f, 0.0f)},
    };
    
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(CPipelineEdge::SPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}
