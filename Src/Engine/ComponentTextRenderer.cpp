#include "ComponentTextRenderer.h"
#include "InterfaceGui.h"

std::vector<VkVertexInputAttributeDescription> CComponentTextRenderer::SPointData::getAttributeDescriptionSet()
{
    CVertexAttributeDescriptor Descriptor;
    Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
    Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
    return Descriptor.generate();
}

std::vector<CComponentTextRenderer::SPointData> CComponentTextRenderer::SPointData::extractFromMeshData(
    const CMeshData& vMeshData)
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

CComponentTextRenderer::CComponentTextRenderer()
{
    m_pFont = CFont::getDefaultFont();
}

void CComponentTextRenderer::setText(std::string vText)
{
    if (m_Text != vText)
    {
        m_Text = vText;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

void CComponentTextRenderer::setFont(sptr<CFont> vFont)
{
    if (vFont != m_pFont)
    {
        m_pFont = vFont;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

void CComponentTextRenderer::setAnchor(glm::vec3 vAnchor)
{
    if (vAnchor != m_Anchor)
    {
        m_Anchor = vAnchor;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

void CComponentTextRenderer::setOffset(glm::vec2 vOffset)
{
    if (vOffset != m_Offset)
    {
        m_Offset = vOffset;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

void CComponentTextRenderer::setScale(float vScale)
{
    if (vScale != m_Scale)
    {
        m_Scale = vScale;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

void CComponentTextRenderer::setLineHeight(float vLineHeight)
{
    if (vLineHeight != m_LineHeight)
    {
        m_LineHeight = vLineHeight;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

void CComponentTextRenderer::setSpacing(float vSpacing)
{
    if (vSpacing != m_Spacing)
    {
        m_Spacing = vSpacing;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

void CComponentTextRenderer::setHorizonAlign(ETextAlign vAlign)
{
    if (vAlign != m_HorizonAlign)
    {
        m_HorizonAlign = vAlign;
        m_TextMeshUpdateEventHandler.trigger();
    }
}

glm::vec3 CComponentTextRenderer::getWorldPosition() const
{
    return m_pParent.lock()->getAbsoluteTranslate() + m_Anchor;
}

sptr<vk::CVertexBuffer> CComponentTextRenderer::generateVertexBuffer(cptr<vk::CDevice> vDevice) const
{
    if (m_Text.empty()) return nullptr;
    CMeshData MeshData = __generateTextMesh();
    auto pVertBuffer = make<vk::CVertexBufferTyped<SPointData>>();
    const auto& Data = SPointData::extractFromMeshData(MeshData);
    pVertBuffer->create(vDevice, Data);
    return pVertBuffer;
}

void CComponentTextRenderer::_renderUIV()
{
    std::string IdPostfix = "##" + std::to_string(reinterpret_cast<int64_t>(this));

    std::string Text = m_Text;
    if (UI::textarea(u8"文本" + IdPostfix, Text)) setText(Text);

    glm::vec3 Anchor = m_Anchor;
    if (UI::drag(u8"锚点" + IdPostfix, Anchor)) setAnchor(Anchor);

    glm::vec2 Offset = m_Offset;
    if (UI::drag(u8"偏移" + IdPostfix, Offset)) setOffset(Offset);

    float Scale = m_Scale;
    if (UI::drag(u8"缩放" + IdPostfix, Scale)) setScale(Scale);

    float LineHeight = m_LineHeight;
    if (UI::drag(u8"行高" + IdPostfix, LineHeight)) setLineHeight(LineHeight);

    float Spacing = m_Spacing;
    if (UI::drag(u8"字距" + IdPostfix, Spacing)) setSpacing(Spacing);
}

SAABB CComponentTextRenderer::getAABBV() const
{
    return SAABB::createByCenterExtent(m_pParent.lock()->getAbsoluteTranslate(), m_pParent.lock()->getAbsoluteScale() * 2.0f);
}

std::string CComponentTextRenderer::_getNameV() const
{ return "Text Renderer"; }

void CComponentTextRenderer::__appendCharMesh(glm::vec2 vCursor, const CFont::SFontDrawInfo& vDrawInfo,
    CMeshData& vioMeshData) const
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

void CComponentTextRenderer::__shiftLineByHorizonAlign(size_t vLineStartIndex, size_t vLineEndIndex,
    float vLineWidth, CMeshData& vioMeshData) const
{
    auto pVertexArray = vioMeshData.getVertexArray();
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

CMeshData CComponentTextRenderer::__generateTextMesh() const
{
    CMeshData MeshData = CMeshData();
    auto pVertexArray = MeshData.getVertexArray();

    glm::vec2 Cursor = m_Offset;
    size_t LineStartVertIndex = 0;
    for (char Char : m_Text)
    {
        if (Char == '\n')
        {
            __shiftLineByHorizonAlign(LineStartVertIndex, pVertexArray->size(), Cursor.x, MeshData);
            Cursor.x = m_Offset.x;
            Cursor.y -= m_LineHeight * m_Scale;
            LineStartVertIndex = pVertexArray->size();
            continue;
        }
        const CFont::SFontDrawInfo& DrawInfo = m_pFont->getCharDrawInfo(Char);

        __appendCharMesh(Cursor, DrawInfo, MeshData);

        Cursor.x += (DrawInfo.Advance + m_Spacing) * m_Scale;
    }

    // shift by align
    __shiftLineByHorizonAlign(LineStartVertIndex, pVertexArray->size(), Cursor.x, MeshData);

    return MeshData;
}
