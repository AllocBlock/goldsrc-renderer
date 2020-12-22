#include "IOImage.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool CIOImage::_readV(std::string vFileName)
{
    __cleanup();
    m_pData = static_cast<void*>(stbi_load(vFileName.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha)); // 强制读取为RGBA
    if (!m_pData)
        throw "image load failed, " + std::string(stbi_failure_reason());
    return true;
}

void CIOImage::__cleanup()
{
    if (m_pData)
        stbi_image_free(m_pData);
}