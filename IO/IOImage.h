#pragma once
#include "IOBase.h"
#include "Pointer.h"

class CIOImage : public CIOBase
{
public:
    _DEFINE_PTR(CIOImage);

    CIOImage() : CIOBase() {}
    CIOImage(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}
    CIOImage(const CIOImage& vObj)
    {
        m_Width = vObj.getWidth();
        m_Height = vObj.getHeight();
        m_ChannelNum = vObj.getChannelNum();
        setData(vObj.getData());
    }
    virtual ~CIOImage() { __cleanup(); }

    size_t getWidth() const { return m_Width; }
    size_t getHeight() const { return m_Height; }
    size_t getChannelNum() const { return m_ChannelNum; }
    const void* getData() const { return m_pData; }

    void setSize(size_t vWidth, size_t vHeight) { m_Width = vWidth; m_Height = vHeight; }
    void setChannelNum(size_t vChannels) { m_ChannelNum = vChannels; }
    void setData(const void* vpData);

    void writePPM(std::filesystem::path vFilePath);

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    void __cleanup();

    size_t m_Width = 0;
    size_t m_Height = 0;
    size_t m_ChannelNum = 0;
    void* m_pData = nullptr;
};