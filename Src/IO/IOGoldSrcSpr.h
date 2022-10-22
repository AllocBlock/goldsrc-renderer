#pragma once
#include "IOBase.h"
#include "IOCommon.h"

struct SSprHeader
{
    char Magic[4];     // "IDSP"
    int32_t Version;   // ��Դ��ʼ��Ϊ2
    int32_t Type;      // 0: vp parallel upright; 1: facing upright; 2: vp parallel; 3: oriented; 4: vp parallel oriented
    int32_t TextureFormat;
    float Radius;      // Bounding Radius
    int32_t MaxWidth;  // ����֡�����Ŀ��
    int32_t MaxHeight; // ����֡�����ĸ߶�
    int32_t FrameNum;  // ֡������
    float BeamLength;  // δ֪
    int32_t SyncType; // 0=synchron 1=random (most commonly synchron)
};

struct SSprPalette
{
    int16_t Size; // ��ɫ���С��ʼ�յ���256
    GoldSrc::SColor ColorSet[256]; // ��ɫ
};

struct SSprPicture
{
    int32_t Group; // ��ţ�
    int32_t OffsetX; // X����ƫ��
    int32_t OffsetY; // Y����ƫ��
    int32_t Width;   // ���
    int32_t Height;  // ����
    std::vector<uint8_t> IndexSet; // ��������
};

// ���� https://moddb.fandom.com/wiki/SPR (quake) �� https://github.com/jpiolho/GoldSrcSprite ��Goldsrc�� ʵ��
// spr�����Ϳɲο� https://the303.org/tutorials/gold_sprite.htm 
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

