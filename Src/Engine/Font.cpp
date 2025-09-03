#include "Font.h"
#include "StaticResource.h"
#include "Common.h"

sptr<CFont> pDefaultFont = nullptr;

sptr<CFont> CFont::getDefaultFont()
{
    if (!pDefaultFont)
    {
        pDefaultFont = make<CFont>("Consolas", 128, true);
    }
    return pDefaultFont;
}

CFont::CFont(std::string vFontName, size_t vFontSize, bool vIsSDF)
{
    m_FontSize = vFontSize;
    std::string Prefix = "Fonts/" + vFontName + (vIsSDF ? "_SDF_" : "_");
    std::string ImageFile = Prefix + std::to_string(vFontSize) + ".png";
    std::string LayoutFile = Prefix + "Layout_" + std::to_string(vFontSize) + ".csv";

    m_pFontImage = StaticResource::loadImage(ImageFile);
    m_FontAtlasSize = glm::vec2(m_pFontImage->getWidth(), m_pFontImage->getHeight());
    auto LayoutFilePath = StaticResource::getAbsPath(LayoutFile);

    if (!std::filesystem::exists(LayoutFilePath))
        throw std::runtime_error("Font layout file not found");
    
    std::string LayoutStr = Common::readFileAsString(LayoutFilePath);

    std::vector<std::string> Lines = Common::split(LayoutStr, "\n");
    for (const auto& Line : Lines)
    {
        if (Line.empty()) continue;
        auto Comps = Common::split(Line, "\t");
        if (Comps.size() != 8 || Comps[0].size() != 1)
            throw std::runtime_error("Font layout file has wrong format");

        char Char = Comps[0][0];
        if (m_CharInfoMap.find(Char) != m_CharInfoMap.end())
            throw std::runtime_error("Font layout file has duplicate characters");

        SCharacterInfo CharInfo;
        CharInfo.Offset = glm::uvec2(std::stoi(Comps[1]), std::stoi(Comps[2]));
        CharInfo.Size = glm::uvec2(std::stoi(Comps[3]), std::stoi(Comps[4]));
        CharInfo.Anchor = glm::vec2(std::stoi(Comps[5]), std::stoi(Comps[6]));
        CharInfo.Advance = std::stoi(Comps[7]);

        m_CharInfoMap[Char] = CharInfo;
    }
}

CFont::SFontDrawInfo CFont::getCharDrawInfo(char vChar)
{
    float NormalizeScale = 2.0f / m_FontSize;
    // TODO: cache draw info
    const auto& Info = m_CharInfoMap.at(vChar);

    SFontDrawInfo DrawInfo;
    DrawInfo.Size = glm::vec2(Info.Size) * NormalizeScale;
    DrawInfo.Offset = Info.Anchor * NormalizeScale;
    DrawInfo.Advance = Info.Advance * NormalizeScale;

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
