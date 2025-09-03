#pragma once
#include "Pipeline.h"
#include <glm/glm.hpp>
#include "Camera.h"
#include "UniformBuffer.h"
#include "IOImage.h"
#include "Sampler.h"

class CPipelineEnvironment : public IPipeline
{
public:
    bool isReady() { return m_Ready; }
    void setEnvironmentMap(sptr<CIOImage> vSkyImage);
    void updateUniformBuffer(uint32_t vImageIndex, sptr<CCamera> vCamera);

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __precalculateIBL(sptr<CIOImage> vSkyImage);
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();

    bool m_Ready = false;

    struct SControl 
    {
        glm::vec3 Rotation;
    } m_Control;

    vk::CSampler m_Sampler;
    sptr<vk::CUniformBuffer> m_pFragUniformBuffer;
    vk::CImage m_EnvironmentImage;
    vk::CImage m_PlaceholderImage;
};


