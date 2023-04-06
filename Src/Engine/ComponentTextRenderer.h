#pragma once
#include "Component.h"
#include "Font.h"
#include "Mesh.h"
#include "VertexAttributeDescriptor.h"
#include "VertexBuffer.h"
#include "Event.h"

enum class ETextAlign
{
    LEFT,
    CENTER,
    RIGHT
};

class CComponentTextRenderer : public IComponent
{
    _DEFINE_UPDATE_EVENT(TextMesh)
    _DEFINE_UPDATE_EVENT(ShaderParam)
public:
    _DEFINE_PTR(CComponentTextRenderer);

    struct SPointData
    {
        glm::vec2 Pos;
        glm::vec2 TexCoord;

        using PointData_t = SPointData;

        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet();
        static std::vector<SPointData> extractFromMeshData(const CMeshData& vMeshData);
    };

    CComponentTextRenderer();

    void setText(std::string vText);

    _DEFINE_GETTER_POINTER(Font, CFont::Ptr);
    _DEFINE_GETTER(Anchor, glm::vec3);
    _DEFINE_GETTER(Offset, glm::vec2);
    _DEFINE_GETTER(Scale, float);
    _DEFINE_GETTER(LineHeight, float);
    _DEFINE_GETTER(Spacing, float);

    void setFont(CFont::Ptr vFont);
    void setAnchor(glm::vec3 vAnchor);
    void setOffset(glm::vec2 vOffset);
    void setScale(float vScale);
    void setLineHeight(float vLineHeight);
    void setSpacing(float vSpacing);
    void setHorizonAlign(ETextAlign vAlign);

    glm::vec3 getWorldPosition() const;

    // TODO: instancing may be better
    vk::CVertexBuffer::Ptr generateVertexBuffer(vk::CDevice::CPtr vDevice) const;

    virtual void _renderUIV() override;
    virtual SAABB getAABBV() const override;

protected:
    virtual std::string _getNameV() const override;

private:
    void __appendCharMesh(glm::vec2 vCursor, const CFont::SFontDrawInfo& vDrawInfo, CMeshData& vioMeshData) const;
    void __shiftLineByHorizonAlign(size_t vLineStartIndex, size_t vLineEndIndex, float vLineWidth, CMeshData& vioMeshData) const;
    CMeshData __generateTextMesh() const;

    glm::vec3 m_Anchor = glm::vec3(0.0f);
    glm::vec2 m_Offset = glm::vec2(0.0f);
    std::string m_Text = "";
    CFont::Ptr m_pFont = nullptr;
    float m_Scale = 1.0f;
    float m_LineHeight = 1.0f;
    float m_Spacing = 0.0f;
    ETextAlign m_HorizonAlign = ETextAlign::CENTER;
};
