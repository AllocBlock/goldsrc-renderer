#pragma once
#include "Image.h"
#include <map>
#include <glm/glm.hpp>
#include <array>

#include "IOImage.h"

// ASCII sdf font atlas
class CFont
{
public:
    _DEFINE_PTR(CFont);
    static CFont::Ptr getDefaultFont();

    struct SFontDrawInfo
    {
        glm::vec2 Size; // normalized by fontsize
        glm::vec2 Offset; // normalized by fontsize
        float Advance; // normalized by fontsize
        std::array<glm::vec2, 4> UVs; // start at left top, counter clockwise
    };

    CFont(std::string vFontName, size_t vFontSize, bool vIsSDF);

    float getFontBaseSize() const { return m_FontSize; }
    CIOImage::CPtr getImage() const { return m_pFontImage; }
    SFontDrawInfo getCharDrawInfo(char vChar);

private:
    struct SCharacterInfo
    {
        glm::uvec2 Offset; // x, y (left to right, top to down)
        glm::uvec2 Size; // width, height
        glm::vec2 Anchor; // deltaX, deltaY start at Offset, x mark the leftest, and y mark the "underline" position
        uint32_t Advance; // next char cursor offset
    };

    CIOImage::Ptr m_pFontImage;
    float m_FontSize;
    glm::vec2 m_FontAtlasSize;
    std::map<char, SCharacterInfo> m_CharInfoMap;
};