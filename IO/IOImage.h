#pragma once
#include "IOBase.h"

class CIOImage : public CIOBase
{
public:
    CIOImage() :CIOBase() {}
    CIOImage(std::string vFileName) :CIOBase(vFileName) {}
    virtual ~CIOImage() { __cleanup(); }

    int getImageWidth() const { return m_Width; }
    int getImageHeight() const { return m_Height; }
    int getImageChannels() const { return m_Channels; }
    const void* getData() const { return m_pData; }

protected:
    virtual bool _readV(std::string vFileName) override;
private:
    void __cleanup();

    int m_Width = 0;
    int m_Height = 0;
    int m_Channels = 0;
    void* m_pData = nullptr;
};