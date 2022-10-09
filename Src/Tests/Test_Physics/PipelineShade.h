#pragma once
#include "IPipeline.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "VertexAttributeDescriptor.h"
#include "Camera.h"
#include "Mesh.h"

#include <glm/ext/matrix_transform.hpp>

class CPipelineShade : public IPipeline
{
public:
    struct SPointData
    {
        glm::vec3 Pos;
        glm::vec3 Normal;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            static std::vector<VkVertexInputAttributeDescription> Result;
            if (Result.empty())
            {
                CVertexAttributeDescriptor Descriptor;
                Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
                Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
                Result = Descriptor.generate();
            }
            return Result;
        }

        static std::vector<SPointData> extractFromMeshData(const CGeneralMeshDataTest& vMeshData)
        {
            auto pVertexArray = vMeshData.getVertexArray();
            auto pNormalArray = vMeshData.getNormalArray();

            size_t NumPoint = pVertexArray->size();
            _ASSERTE(NumPoint == pNormalArray->size());

            std::vector<CPipelineShade::SPointData> PointData(NumPoint);
            for (size_t i = 0; i < NumPoint; ++i)
            {
                PointData[i].Pos = pVertexArray->get(i);
                PointData[i].Normal = pNormalArray->get(i);
            }
            return PointData;
        }
    };

    struct SPushConstant
    {
        glm::mat4 Model;
        glm::mat4 NormalModel;
    };

    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void updatePushConstant(VkCommandBuffer vCommandBuffer, const glm::mat4& vModelMatrix);

protected:
    virtual void _initPushConstantV(VkCommandBuffer vCommandBuffer) override
    {
        updatePushConstant(vCommandBuffer, glm::identity<glm::mat4>());
    }
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/shaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/shaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV() override;

    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __destroyResources();
    
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CImage m_PlaceholderImage;
};

