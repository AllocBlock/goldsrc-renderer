#pragma once

#include "IOBase.h"

#include <vector>
#include <array>
#include <optional>

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
    std::string NameUpperCase;
    uint32_t Width;
    uint32_t Height;
    std::array<uint32_t, 4> ImageOffsets;
    std::array<std::vector<uint8_t>, 4> ImageDatas;
    std::array<WadColor, 256> Palette;

    void read(std::ifstream& vFile, uint32_t vOffset);

    friend bool operator < (const SWadTexture& vA, const SWadTexture& vB)
    {
        return vA.NameUpperCase < vB.NameUpperCase;
    }

    friend bool operator == (const SWadTexture& vA, const SWadTexture& vB)
    {
        return vA.NameUpperCase == vB.NameUpperCase;
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
    CIOGoldsrcWad(std::filesystem::path vFilePath) :CIOBase(vFilePath) {}

    size_t getTextureNum() const;
    std::optional<size_t> findTexture(std::string vName) const;
    std::string getTextureName(size_t vTexIndex) const;
    std::string getTextureNameFormatted(size_t vTexIndex) const;
    void getTextureSize(size_t vTexIndex, uint32_t& voWidth, uint32_t& voHeight) const;
    void getRawRGBAPixels(size_t vTexIndex, void* vopData) const;

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    SWadHeader m_Header = {};
    std::vector<SWadLump> m_LumpsList;
    std::vector<SWadTexture> m_TexturesList;
};