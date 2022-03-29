#pragma once
#include "Pipeline.h"
#include <glm/glm.hpp>
#include "Camera.h"
#include "UniformBuffer.h"
#include "IOImage.h"
#include "FullScreenPoint.h"

class CPipelineEnvironment : public IPipeline
{
public:
    bool isReady() { return m_Ready; }
    void setEnvironmentMap(CIOImage::Ptr vHDRI);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::Ptr vCamera);
    void destroy();

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/envVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/envFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();

    bool m_Ready = false;

    struct SControl 
    {
        glm::vec3 Rotation;
    } m_Control;

    VkSampler m_Sampler = VK_NULL_HANDLE;
    std::vector<vk::CUniformBuffer::Ptr> m_FragUBSet;
    vk::CImage::Ptr m_pEnvironmentImage = nullptr;
    vk::CImage::Ptr m_pPlaceholderImage = nullptr;
};


