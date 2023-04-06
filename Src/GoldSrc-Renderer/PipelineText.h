#pragma once
#include "Pipeline.h"
#include "IOImage.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"
#include "ComponentTextRenderer.h"

#include "Actor.h"

class CPipelineText : public IPipeline
{
public:
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void addTextComponent(CComponentTextRenderer::Ptr vTextRenderer);
    void clearTextComponent();
    bool doesNeedRerecord(size_t vImageIndex);
    bool recordCommand(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _initPushConstantV(CCommandBuffer::Ptr vCommandBuffer) override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __markNeedRerecord()
    {
        for (size_t i = 0; i < m_NeedRerecordSet.size(); ++i)
            m_NeedRerecordSet[i] = true;
    }

    vk::CSampler m_Sampler;
    vk::CImage m_FontImage;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    
    std::vector<CComponentTextRenderer::Ptr> m_TextRendererSet;
    std::vector<vk::CVertexBuffer::Ptr> m_VertBufferSet;
    std::vector<bool> m_NeedUpdateVertBufferSet;
    std::vector<bool> m_NeedRerecordSet;
};

