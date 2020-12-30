#pragma once

#include "IOBase.h"

#include <vector>
#include <array>

struct WadColor
{
    uint8_t R, G, B;
};

struct SWadHeader
{
    char FileId[4];
    uint32_t TexturesNum;
    uint32_t LumpsListOffset;

    void read(std::ifstream& vFile);
};

struct SWadLump
{
    uint32_t Offset;
    uint32_t CompressedSize;
    uint32_t FullSize;
    char Type; // only 0x43 for now
    char CompressedType;
    std::string NameOrigin;

    void read(std::ifstream& vFile);
};

struct SWadTexture
{
    std::string NameOrigin;
    std::string NameFormatted;
    uint32_t Width;
    uint32_t Height;
    std::array<uint32_t, 4> ImageOffsets;
    std::array<std::vector<uint8_t>, 4> ImageDatas;
    std::array<WadColor, 256> Palette;

    void read(std::ifstream& vFile, uint32_t vOffset);
    static std::string __toUpperCase(std::string vStr);

    friend bool operator < (const SWadTexture& vA, const SWadTexture& vB)
    {
        return vA.NameFormatted < vB.NameFormatted;
    }

    friend bool operator == (const SWadTexture& vA, const SWadTexture& vB)
    {
        return vA.NameFormatted == vB.NameFormatted;
    }
};

/***************************************************************
 * Class for wad file reading.
 * Only support 0x43 miptex reading for now (other type should not be used in goldsrc map itself, but only used in engine).
 **************************************************************/
class CIOGoldsrcWad : public CIOBase 
{
public:
    CIOGoldsrcWad() :CIOBase() {}
    CIOGoldsrcWad(std::string vFileName) :CIOBase(vFileName) {}

    size_t getTextureNum();
    std::string getTextureName(size_t vTexIndex);
    std::string getTextureNameFormatted(size_t vTexIndex);
    void getTextureSize(size_t vTexIndex, uint32_t& voWidth, uint32_t& voHeight);
    void getRawRGBAPixels(size_t vTexIndex, void*& vopData);

protected:
    virtual bool _readV(std::string vFileName) override;

private:
    SWadHeader m_Header = {};
    std::vector<SWadLump> m_LumpsList;
    std::vector<SWadTexture> m_TexturesList;
};