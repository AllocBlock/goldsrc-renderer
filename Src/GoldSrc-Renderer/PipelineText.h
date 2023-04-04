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
    void drawTextActor(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex, CActor::Ptr vActor);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _initPushConstantV(CCommandBuffer::Ptr vCommandBuffer) override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CImage m_FontImage;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    
    std::map<CComponentTextRenderer::CPtr, vk::CVertexBuffer::Ptr> m_TextCompVertBufferMap;
};

