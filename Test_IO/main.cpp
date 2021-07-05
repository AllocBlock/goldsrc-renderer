#include "IOGoldSrcSpr.h"
#include "IOImage.h"

void saveSpr(const CIOGoldSrcSpr& vSpr, std::string vFilePrefix)
{
    size_t FrameNum = vSpr.getFrameNum();
    for (size_t i = 0; i < FrameNum; ++i)
    {
        uint32_t Width = 0, Height = 0;
        vSpr.getFrameSize(i, Width, Height);
        void* pData = new uint8_t[Width * Height * 8];
        vSpr.getFrameRGBAPixels(i, pData);

        CIOImage Image;
        Image.setImageSize(Width, Height);
        Image.setData(pData);
        std::string FileName = vFilePrefix;
        if (FrameNum > 1)
            FileName += "_" + std::to_string(i + 1);
        FileName += ".ppm";
        Image.writePPM(FileName);

        delete[] pData;
    }
}

int main()
{
    CIOGoldSrcSpr Spr1;
    Spr1.read("spr/emc2t.spr");
    saveSpr(Spr1, "emc2t");

    CIOGoldSrcSpr Spr2;
    Spr2.read("spr/explode1.spr");
    saveSpr(Spr2, "explode1");
    return 0;
}