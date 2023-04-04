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

    CFont(std::string vFontName, size_t vFontSize, bool vIsSDF);

    struct SFontDrawInfo
    {
        glm::vec2 Size; // normalized by fontsize
        glm::vec2 Offset; // normalized by fontsize
        float Advance; // normalized by fontsize
        std::array<glm::vec2, 4> UVs; // start at left top, counter clockwise
    };

    CIOImage::CPtr getImage()
    {
        return m_pFontImage;
    }
    
    SFontDrawInfo getCharDrawInfo(char vChar)
    {
        // TODO: cache draw info
        const auto& Info = m_CharInfoMap.at(vChar);

        SFontDrawInfo DrawInfo;
        DrawInfo.Size = glm::vec2(Info.Size) / m_FontSize;
        DrawInfo.Offset = Info.Anchor / m_FontSize;
        DrawInfo.Advance = Info.Advance / m_FontSize;

        glm::vec2 LeftTop = glm::vec2(Info.Offset) / m_FontAtlasSize;
        glm::vec2 RightBottom = glm::vec2(Info.Offset + Info.Size) / m_FontAtlasSize;
        DrawInfo.UVs = {
            LeftTop,
            glm::vec2(LeftTop.x, RightBottom.y),
            RightBottom,
            glm::vec2(RightBottom.x, LeftTop.y),
        };
        return DrawInfo;
    }

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