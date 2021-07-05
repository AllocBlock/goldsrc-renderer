#pragma once
#include "IOBase.h"

struct SSprHeader
{
    char Magic[4];     // "IDSP"
    int32_t Version;   // 1：索引模式； 2：直接存储颜色；
    int32_t Type;      // 0: vp parallel upright; 1: facing upright; 2: vp parallel; 3: oriented; 4: vp parallel oriented
    float Radius;      // Bounding Radius
    int32_t MaxWidth;  // 所以帧中最大的宽度
    int32_t MaxHeight; // 所以帧中最大的高度
    int32_t FrameNum;  // 帧的总数
    float BeamLength;  // 未知
    int32_t SynchType; // 0=synchron 1=random (most commonly synchron)
};

struct SSprPicture
{
    int32_t OffsetX; // horizontal offset, in 3D space
    int32_t OffsetY; // vertical offset, in 3D space
    int32_t Width;   // width of the picture
    int32_t Height;  // height of the picture
    std::vector<uint8_t> DataSet; // 数据
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
    size_t getFrameTime(size_t vIndex) const;
    void getFrameRGBAPixels(size_t vIndex, void* voData) const;

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    SSprPicture readPicture(std::ifstream& voFile);

    SSprHeader m_Header = {};
    std::vector<SSprPicture> m_FrameSet;
    std::vector<float> m_TimeSet;
};

