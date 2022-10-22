#pragma once
#include "IOBase.h"
#include "IOCommon.h"

struct SSprHeader
{
    char Magic[4];     // "IDSP"
    int32_t Version;   // 金源中始终为2
    int32_t Type;      // 0: vp parallel upright; 1: facing upright; 2: vp parallel; 3: oriented; 4: vp parallel oriented
    int32_t TextureFormat;
    float Radius;      // Bounding Radius
    int32_t MaxWidth;  // 所以帧中最大的宽度
    int32_t MaxHeight; // 所以帧中最大的高度
    int32_t FrameNum;  // 帧的总数
    float BeamLength;  // 未知
    int32_t SyncType; // 0=synchron 1=random (most commonly synchron)
};

struct SSprPalette
{
    int16_t Size; // 调色板大小，始终等于256
    GoldSrc::SColor ColorSet[256]; // 颜色
};

struct SSprPicture
{
    int32_t Group; // 组号？
    int32_t OffsetX; // X方向偏移
    int32_t OffsetY; // Y方向偏移
    int32_t Width;   // 宽度
    int32_t Height;  // 长度
    std::vector<uint8_t> IndexSet; // 索引数据
};

// 基于 https://moddb.fandom.com/wiki/SPR (quake) 和 https://github.com/jpiolho/GoldSrcSprite （Goldsrc） 实现
// spr的类型可参考 https://the303.org/tutorials/gold_sprite.htm 
class CIOGoldSrcSpr : public CIOBase
{
public:
    CIOGoldSrcSpr() : CIOBase() {}
    CIOGoldSrcSpr(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}

    size_t getFrameNum() const;
    void getFrameSize(size_t vIndex, uint32_t& voWidth, uint32_t& voHeight) const;
    void getFrameRGBAPixels(size_t vIndex, void* voData) const;
    int getType() const;

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    void readPicture(std::ifstream& voFile, SSprPicture& voPicture);

    SSprHeader m_Header = {};
    SSprPalette m_Palatte;
    std::vector<SSprPicture> m_FrameSet;
};

