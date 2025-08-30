#pragma once
#include "Pipeline.h"
#include "IOImage.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"
#include "ComponentTextRenderer.h"

class CPipelineText : public IPipeline
{
public:
    void updateUniformBuffer(CCamera::CPtr vCamera);
    void addTextComponent(CComponentTextRenderer::Ptr vTextRenderer);
    void clearTextComponent();
    bool doesNeedRerecord();
    bool recordCommand(CCommandBuffer::Ptr vCommandBuffer);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _initPushConstantV(CCommandBuffer::Ptr vCommandBuffer) override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __markNeedRerecord();

    vk::CSampler m_Sampler;
    vk::CImage m_FontImage;
    vk::CUniformBuffer::Ptr m_pVertUniformBuffer;
    
    std::vector<CComponentTextRenderer::Ptr> m_TextRendererSet;
    std::vector<vk::CVertexBuffer::Ptr> m_VertBufferSet;
    std::vector<bool> m_NeedUpdateVertBufferSet;
    bool m_NeedRerecord;
};

