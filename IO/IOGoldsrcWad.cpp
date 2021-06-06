#include "IOGoldsrcWad.h"

#include <fstream>
#include <algorithm>

const size_t BSP_MAX_NAME_LENGTH = 16;

bool CIOGoldsrcWad::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File;
    File.open(vFilePath.string(), std::ios::in | std::ios::binary);
    if (!File.is_open())
    {
        Common::Log::log(u8"打开文件 [" + vFilePath.u8string() + u8"] 失败，无权限或文件不存在");
        return false;
    }

    m_Header.read(File);

    File.seekg(m_Header.LumpsListOffset);
    while (true)
    {
        SWadLump Lump;
        Lump.read(File);
        if (File.eof()) break;
        m_LumpsList.push_back(Lump);
    }
    File.clear(); // reach EOF, need clear() before reuse

    for (SWadLump Lump : m_LumpsList)
    {
        SWadTexture Texture;
        Texture.read(File, Lump.Offset);
        m_TexturesList.push_back(Texture);
    }

    return true;
}

size_t CIOGoldsrcWad::getTextureNum() const
{
    return m_TexturesList.size();
}

std::optional<size_t> CIOGoldsrcWad::findTexture(std::string vName) const
{
    vName = toUpperCase(vName);
    for (size_t i = 0; i < m_TexturesList.size(); i++)
    {
        if (m_TexturesList[i].NameUpperCase == vName) return i;
    }
    return std::nullopt;
}

std::string CIOGoldsrcWad::getTextureName(size_t vTexIndex) const
{
    _ASSERTE(vTexIndex >= 0 && vTexIndex < m_TexturesList.size());
    return m_TexturesList[vTexIndex].NameOrigin;
}

std::string CIOGoldsrcWad::getTextureNameFormatted(size_t vTexIndex) const
{
    _ASSERTE(vTexIndex >= 0 && vTexIndex < m_TexturesList.size());
    return m_TexturesList[vTexIndex].NameUpperCase;
}

void CIOGoldsrcWad::getTextureSize(size_t vTexIndex, uint32_t& voWidth, uint32_t& voHeight) const
{
    _ASSERTE(vTexIndex >= 0 && vTexIndex < m_TexturesList.size());
    voWidth = m_TexturesList[vTexIndex].Width;
    voHeight = m_TexturesList[vTexIndex].Height;
}

void CIOGoldsrcWad::getRawRGBAPixels(size_t vTexIndex, void* vopData) const
{
    _ASSERTE(vTexIndex >= 0 && vTexIndex < m_TexturesList.size());
    bool HasAlphaIndex = false;
    if (m_TexturesList[vTexIndex].NameOrigin[0] == '{')
    {
        HasAlphaIndex = true;
    }

    unsigned char* pIter = static_cast<unsigned char*>(vopData);
    for (uint8_t PalatteIndex : m_TexturesList[vTexIndex].ImageDatas[0])
    {
        if (HasAlphaIndex && PalatteIndex == 0xff) // transparent color
        {
            *pIter++ = static_cast<unsigned char>(0x00);
            *pIter++ = static_cast<unsigned char>(0x00);
            *pIter++ = static_cast<unsigned char>(0x00);
            *pIter++ = static_cast<unsigned char>(0x00);
        }
        else
        {
            IOCommon::SGoldSrcColor PixelColor = m_TexturesList[vTexIndex].Palette[PalatteIndex];
            *pIter++ = static_cast<unsigned char>(PixelColor.R);
            *pIter++ = static_cast<unsigned char>(PixelColor.G);
            *pIter++ = static_cast<unsigned char>(PixelColor.B);
            *pIter++ = static_cast<unsigned char>(0xff);
        }
    }
}

void SWadHeader::read(std::ifstream& vFile)
{
    vFile.read(reinterpret_cast<char*>(this), sizeof(SWadHeader));
}

void SWadLump::read(std::ifstream& vFile)
{
    vFile.read(reinterpret_cast<char*>(&Offset),           sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&CompressedSize),   sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&FullSize),         sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&Type),             sizeof(char));
    vFile.read(reinterpret_cast<char*>(&CompressedType),   sizeof(char));
    vFile.get(); vFile.get(); // padding
    char TextureNameBuffer[BSP_MAX_NAME_LENGTH];
    vFile.read(reinterpret_cast<char*>(TextureNameBuffer), sizeof(char[BSP_MAX_NAME_LENGTH]));
    NameOrigin = TextureNameBuffer;
}

void SWadTexture::read(std::ifstream& vFile, uint32_t vOffset)
{
    vFile.seekg(vOffset);
    char StringBuffer[BSP_MAX_NAME_LENGTH];

    vFile.read(reinterpret_cast<char*>(StringBuffer),     sizeof(char[BSP_MAX_NAME_LENGTH]));
    NameOrigin = StringBuffer;
    NameUpperCase = CIOBase::toUpperCase(NameOrigin);
    vFile.read(reinterpret_cast<char*>(&Width),           sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&Height),          sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&ImageOffsets[0]), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&ImageOffsets[1]), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&ImageOffsets[2]), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&ImageOffsets[3]), sizeof(uint32_t));

    uint32_t TexSize = Height * Width;
    for (int i = 0; i < 4; ++i)
    {
        vFile.seekg(static_cast<uint64_t>(vOffset) + ImageOffsets[i], std::ios::beg);
        ImageDatas[i].resize(TexSize);
        vFile.read(reinterpret_cast<char*>(ImageDatas[i].data()), TexSize);
        TexSize /= 4;
    }

    vFile.get(); vFile.get(); // 0x00, 0x01

    for (int i = 0; i < Palette.size(); i++)
    {
        IOCommon::SGoldSrcColor Color;
        Color.R = vFile.get();
        Color.G = vFile.get();
        Color.B = vFile.get();
        Palette[i] = Color;
    }
}

