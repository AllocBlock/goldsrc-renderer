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
    void updateUniformBuffer(cptr<CCamera> vCamera);
    void addTextComponent(sptr<CComponentTextRenderer> vTextRenderer);
    void clearTextComponent();
    bool doesNeedRerecord();
    bool recordCommand(sptr<CCommandBuffer> vCommandBuffer);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _initPushConstantV(sptr<CCommandBuffer> vCommandBuffer) override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __markNeedRerecord();

    vk::CSampler m_Sampler;
    vk::CImage m_FontImage;
    sptr<vk::CUniformBuffer> m_pVertUniformBuffer;
    
    std::vector<sptr<CComponentTextRenderer>> m_TextRendererSet;
    std::vector<sptr<vk::CVertexBuffer>> m_VertBufferSet;
    std::vector<bool> m_NeedUpdateVertBufferSet;
    bool m_NeedRerecord;
};

