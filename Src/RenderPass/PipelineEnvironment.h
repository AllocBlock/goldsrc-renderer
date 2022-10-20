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
    void setEnvironmentMap(CIOImage::Ptr vSkyImage);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::Ptr vCamera);

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    void __precalculateIBL(CIOImage::Ptr vSkyImage);
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();

    bool m_Ready = false;

    struct SControl 
    {
        glm::vec3 Rotation;
    } m_Control;

    vk::CSampler m_Sampler;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUBSet;
    vk::CImage m_EnvironmentImage;
    vk::CImage m_PlaceholderImage;
};


