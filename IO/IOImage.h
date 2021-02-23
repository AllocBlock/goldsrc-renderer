#pragma once
#include "IOBase.h"

class CIOImage : public CIOBase
{
public:
    CIOImage() : CIOBase() {}
    CIOImage(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}
    CIOImage(const CIOImage& vObj)
    {
        m_Width = vObj.getImageWidth();
        m_Height = vObj.getImageHeight();
        m_Channels = vObj.getImageChannels();
        setData(vObj.getData());
    }
    virtual ~CIOImage() { __cleanup(); }

    int getImageWidth() const { return m_Width; }
    int getImageHeight() const { return m_Height; }
    int getImageChannels() const { return m_Channels; }
    const void* getData() const { return m_pData; }

    void setImageSize(int vWidth, int vHeight) { m_Width = vWidth; m_Height = vHeight; }
    void setImageChannels(int vChannels) { m_Channels = vChannels; }
    void setData(const void* vpData);

    void writePPM(std::filesystem::path vFilePath);

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    void __cleanup();

    int m_Width = 0;
    int m_Height = 0;
    int m_Channels = 0;
    void* m_pData = nullptr;
};