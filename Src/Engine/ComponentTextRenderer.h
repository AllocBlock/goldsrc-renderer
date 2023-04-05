#pragma once
#include "Component.h"
#include "Font.h"
#include "Mesh.h"
#include "VertexAttributeDescriptor.h"
#include "VertexBuffer.h"

enum class ETextAlign
{
    LEFT,
    CENTER,
    RIGHT
};

class CComponentTextRenderer : public IComponent
{
public:
    _DEFINE_PTR(CComponentTextRenderer);

    struct SPointData
    {
        glm::vec2 Pos;
        glm::vec2 TexCoord;

        using PointData_t = SPointData;

        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
            return Descriptor.generate();
        }

        static std::vector<SPointData> extractFromMeshData(const CMeshData& vMeshData)
        {
            auto pVertexArray = vMeshData.getVertexArray();
            auto pTexCoordArray = vMeshData.getTexCoordArray();

            size_t NumPoint = pVertexArray->size();
            _ASSERTE(NumPoint == pTexCoordArray->size());

            std::vector<SPointData> PointData(NumPoint);
            for (size_t i = 0; i < NumPoint; ++i)
            {
                PointData[i].Pos = glm::vec2(pVertexArray->get(i));
                PointData[i].TexCoord = pTexCoordArray->get(i);
            }
            return PointData;
        }
    };

    CComponentTextRenderer()
    {
        m_pFont = CFont::getDefaultFont();
    }

    void setText(std::string vText)
    {
        if (m_Text != vText)
        {
            m_Text = vText;
            __generateTextMesh();
        }
    }

    _DEFINE_GETTER_SETTER_POINTER(Font, CFont::Ptr);
    _DEFINE_GETTER_SETTER(Anchor, glm::vec3);
    _DEFINE_GETTER_SETTER(Offset, glm::vec2);
    _DEFINE_GETTER_SETTER(Scale, float);

    glm::vec3 getWorldPosition() const
    {
        return m_pParent.lock()->getAbsoluteTranslate() + m_Anchor;
    }

    // TODO: instancing may be better
    vk::CVertexBuffer::Ptr generateVertexBuffer(vk::CDevice::CPtr vDevice) const
    {
        auto pVertBuffer = make<vk::CVertexBufferTyped<SPointData>>();
        const auto& Data = SPointData::extractFromMeshData(m_MeshData);
        pVertBuffer->create(vDevice, Data);
        return pVertBuffer;
    }

    virtual SAABB getAABBV() const override
    {
        return SAABB::createByCenterExtent(m_pParent.lock()->getAbsoluteTranslate(), m_pParent.lock()->getAbsoluteScale() * 2.0f);
    }

protected:
    virtual std::string _getNameV() const override { return "Text Renderer"; }

private:
    void __appendCharMesh(glm::vec2 vCursor, const CFont::SFontDrawInfo& vDrawInfo, CMeshData& vioMeshData)
    {
        auto pVertexArray = vioMeshData.getVertexArray();
        auto pTexCoordArray = vioMeshData.getTexCoordArray();

        float Width = m_Scale * vDrawInfo.Size.x;
        float Height = m_Scale * vDrawInfo.Size.y;

        glm::vec2 Start = vCursor + vDrawInfo.Offset * m_Scale;

        std::vector<glm::vec2> PosSet =
        {
            Start + glm::vec2(0, 0),
            Start + glm::vec2(0, -Height),
            Start + glm::vec2(Width, -Height),
            Start + glm::vec2(Width, 0),
        };

        std::vector<int> Indices = { 0, 1, 2, 0, 2, 3 };

        for (auto Index : Indices)
        {
            pVertexArray->append(glm::vec3(PosSet[Index], 0.0));
            pTexCoordArray->append(vDrawInfo.UVs[Index]);
        }
    }

    void __shiftLineByHorizonAlign(size_t vLineStartIndex, size_t vLineEndIndex, float vLineWidth) const
    {
        auto pVertexArray = m_MeshData.getVertexArray();
        _ASSERTE(vLineEndIndex <= pVertexArray->size());
        // shift by align
        for (size_t i = vLineStartIndex; i < vLineEndIndex; ++i)
        {
            float Shift = 0.0f;
            if (m_HorizonAlign == ETextAlign::CENTER)
                Shift = -vLineWidth * 0.5;
            else if (m_HorizonAlign == ETextAlign::RIGHT)
                Shift = -vLineWidth;

            glm::vec3 Pos = pVertexArray->get(i);
            Pos.x += Shift;
            pVertexArray->set(i, Pos);
        }
    }

    void __generateTextMesh()
    {
        m_MeshData = CMeshData();
        auto pVertexArray = m_MeshData.getVertexArray();

        glm::vec2 Cursor = m_Offset;
        size_t LineStartVertIndex = 0;
        for (char Char : m_Text)
        {
            if (Char == '\n')
            {
                __shiftLineByHorizonAlign(LineStartVertIndex, pVertexArray->size(), Cursor.x);
                Cursor.x = 0.0f;
                Cursor.y += m_LineHeight;
                LineStartVertIndex = pVertexArray->size();
                continue;
            }
            const CFont::SFontDrawInfo& DrawInfo = m_pFont->getCharDrawInfo(Char);

            __appendCharMesh(Cursor, DrawInfo, m_MeshData);

            Cursor.x += (DrawInfo.Advance + m_Spacing) * m_Scale;
        }

        // shift by align
        __shiftLineByHorizonAlign(LineStartVertIndex, pVertexArray->size(), Cursor.x);
    }

    glm::vec3 m_Anchor = glm::vec3(0.0f);
    glm::vec2 m_Offset = glm::vec2(0.0f);
    std::string m_Text = "";
    CFont::Ptr m_pFont = nullptr;
    float m_FontSize = 32.0f;
    float m_Scale = 1.0f;
    float m_LineHeight = 40.0f;
    float m_Spacing = 0.0f;
    ETextAlign m_HorizonAlign = ETextAlign::CENTER;

    CMeshData m_MeshData;
};
