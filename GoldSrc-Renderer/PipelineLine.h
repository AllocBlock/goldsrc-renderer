#pragma once
#include "PipelineBase.h"

#include <map>
#include <glm/glm.hpp>

enum class EGuiObjectType
{
    LINE
};

struct SGuiObject
{
    EGuiObjectType Type = EGuiObjectType::LINE;
    std::vector<glm::vec3> Data;
};

class CPipelineLine : public CPipelineBase
{
public:
    void destroy();
    void updateDescriptorSet();
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex);
    void setObject(std::string vName, std::shared_ptr<SGuiObject> vObject);
    void removeObject(std::string vName);

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "../Renderer/shader/lineVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "../Renderer/shader/lineFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __updateVertexBuffer();

    std::map<std::string, std::shared_ptr<SGuiObject>> m_NameObjectMap;
    size_t m_VertexNum = 0;
    Common::SBufferPack m_VertexDataPack;
    std::vector<Common::SBufferPack> m_VertUniformBufferPacks;
};

