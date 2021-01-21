#include "IOImage.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void CIOImage::setData(const void* vpData)
{
    if (m_pData) STBI_FREE(m_pData);
    size_t DataSize = static_cast<size_t>(4) * m_Width * m_Height;
    m_pData = STBI_MALLOC(DataSize);
    memcpy_s(m_pData, DataSize, vpData, DataSize);
}

void CIOImage::writePPM(std::filesystem::path vFilePath)
{
    uint8_t* pIter = reinterpret_cast<uint8_t*>(m_pData);

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

bool CIOImage::_readV(std::filesystem::path vFilePath)
{
    __cleanup();
    m_pData = static_cast<void*>(stbi_load(vFilePath.string().c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha)); // 强制读取为RGBA
    if (!m_pData)
        throw std::runtime_error(u8"图片读取失败：" + std::string(stbi_failure_reason()));
    return true;
}

void CIOImage::__cleanup()
{
    if (m_pData)
        stbi_image_free(m_pData);
}