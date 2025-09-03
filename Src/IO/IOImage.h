#pragma once
#include "IOBase.h"
#include "Pointer.h"

// FIXME: BRG only on little endian OS 
enum class EPixelFormat
{
    UNKNOWN,
    RGBA8,
    RGBA32,
};

class CIOImage : public CIOBase
{
public:
    
    CIOImage() : CIOBase() {}
    CIOImage(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}
    CIOImage(const CIOImage& vObj)
    {
        m_Name = vObj.getName();
        m_Width = vObj.getWidth();
        m_Height = vObj.getHeight();
        m_ChannelNum = vObj.getChannelNum();
        setData(vObj.getData());
    }
    virtual ~CIOImage() { __cleanup(); }

    std::string getName() const { return m_Name; }
    size_t getWidth() const { return m_Width; }
    size_t getHeight() const { return m_Height; }
    size_t getChannelNum() const { return m_ChannelNum; }
    size_t getBitPerPixel() const;
    size_t getBitDepth() const;
    EPixelFormat getPixelFormat() const { return m_PixelFormat; }
    size_t getDataSize() const { return m_Width * m_Height * getBitPerPixel(); }
    const void* getData() const { return m_Data.data(); }

    void setName(std::string vName) { m_Name = vName; }
    void setSize(size_t vWidth, size_t vHeight) { m_Width = vWidth; m_Height = vHeight; }
    void setChannelNum(size_t vChannels) { m_ChannelNum = vChannels; }
    void setData(const void* vpData);

    void writePPM(std::filesystem::path vFilePath);
    void writeBMP(std::filesystem::path vFilePath);

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    using byte = uint8_t;

    bool __readImageStb();
    bool __readImageTinyexr();
    void __cleanup();

    std::string m_Name = "";
    size_t m_Width = 0;
    size_t m_Height = 0;
    size_t m_ChannelNum = 4;
    EPixelFormat m_PixelFormat = EPixelFormat::RGBA8;
    std::vector<byte> m_Data;
};