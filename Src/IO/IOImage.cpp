#include "IOImage.h"
#include <algorithm>
#define __STDC_LIB_EXT1__ // _s function
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "tinyexr.h"

void CIOImage::setData(const void* vData)
{
    size_t DataSize = getDataSize();
    m_Data.resize(DataSize, 0);
    byte* pData = (byte*) vData;
    std::copy(pData, pData + DataSize, std::begin(m_Data));
}

size_t CIOImage::getBitDepth() const
{
    switch (m_PixelFormat)
    {
    case EPixelFormat::RGBA8: return 8;
    case EPixelFormat::RGBA32: return 32;
    case EPixelFormat::UNKNOWN:
    default:
        return 0;
    }
}
size_t CIOImage::getBitPerPixel() const
{
    return getBitDepth() / 8 * m_ChannelNum;
}


void CIOImage::writePPM(std::filesystem::path vFilePath)
{
    _ASSERTE(m_PixelFormat == EPixelFormat::RGBA8);
    uint8_t* pIter = reinterpret_cast<uint8_t*>(m_Data.data());

    std::ofstream File(vFilePath, std::ios::out);
    File << "P3" << std::endl;
    File << m_Width << " " << m_Height << std::endl;
    File << 255 << std::endl;
    for (int i = 0; i < m_Height; ++i)
    {
        for (int k = 0; k < m_Width; ++k)
        {
            File << std::to_string(pIter[(i * m_Width + k) * 4]) << ' ';
            File << std::to_string(pIter[(i * m_Width + k) * 4 + 1]) << ' ';
            File << std::to_string(pIter[(i * m_Width + k) * 4 + 2]) << ' ';
        }
        File << std::endl;
    }
    File.close();
}

void CIOImage::writeBMP(std::filesystem::path vFilePath)
{
    _ASSERTE(m_PixelFormat == EPixelFormat::RGBA8);
    stbi_write_bmp(vFilePath.string().c_str(), m_Width, m_Height, 4, m_Data.data());
}

bool CIOImage::_readV(std::filesystem::path vFilePath)
{
    __cleanup();

    std::string Ext = vFilePath.extension().string();

    std::transform(Ext.begin(), Ext.end(), Ext.begin(), [](unsigned char c) { return std::tolower(c); });

    if (Ext == ".exr")
        return __readImageTinyexr();
    else
        return __readImageStb();
}

bool CIOImage::__readImageStb()
{
    int Width, Height, ChannelNum;
    byte* pData = stbi_load(m_FilePath.string().c_str(), &Width, &Height, &ChannelNum, STBI_rgb_alpha); // Ç¿ÖÆ¶ÁÈ¡ÎªRGBA
    if (!pData)
        throw std::runtime_error(u8"Í¼Æ¬¶ÁÈ¡Ê§°Ü£º" + std::string(stbi_failure_reason()));

    m_Width = Width;
    m_Height = Height;
    m_ChannelNum = 4;
    m_PixelFormat = EPixelFormat::RGBA8;
    setData(pData);

    stbi_image_free(pData);

    return true;
}

bool CIOImage::__readImageTinyexr()
{
    float* pData; // width * height * RGBA
    int Width, Height;
    const char* pErr = nullptr;

    int Res = LoadEXR(&pData, &Width, &Height, m_FilePath.string().c_str(), &pErr);

    if (Res != TINYEXR_SUCCESS)
    {
        free(pData);
        throw std::runtime_error(u8"Í¼Æ¬¶ÁÈ¡Ê§°Ü£º" + (pErr ? std::string(pErr) : u8"Î»ÖÃ´íÎó"));
    }

    m_Width = Width;
    m_Height = Height;
    m_ChannelNum = 4;
    m_PixelFormat = EPixelFormat::RGBA32;
    setData(pData);
    
    free(pData);
}

void CIOImage::__cleanup()
{
    m_Data.clear();
}
