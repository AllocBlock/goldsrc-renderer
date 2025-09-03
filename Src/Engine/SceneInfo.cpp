#include "SceneInfo.h"

#include <filesystem>

void CLightmap::clear()
{
    m_LightmapImages.clear();
    m_TotalWidth = m_TotalHeight = 0;
    m_StartHeight = m_StartWidth = m_CurrentRowHeight = 0;
    m_Offsets.clear();
}

size_t CLightmap::appendLightmap(sptr<CIOImage> vImage)
{
    if (m_Channel != vImage->getChannelNum())
    {
        throw std::runtime_error(u8"错误：Lightmap图像的通道数量错误，要求为" + std::to_string(m_Channel) + u8"，实际为" + std::to_string(vImage->getChannelNum()));
    }
    size_t LightmapIndex = m_LightmapImages.size();

    if (m_StartWidth + vImage->getWidth() > m_MaxWidth) // next row
    {
        m_StartHeight += m_CurrentRowHeight;
        m_StartWidth = m_CurrentRowHeight = 0;
    }

    // simply place texture in a row
    m_Offsets.emplace_back(std::make_pair(m_StartWidth, m_StartHeight));
    m_StartWidth += vImage->getWidth();
    m_TotalWidth = std::max<size_t>(m_TotalWidth, m_StartWidth);
    if (vImage->getHeight() > m_CurrentRowHeight)
    {
        m_TotalHeight += vImage->getHeight() - m_CurrentRowHeight;
        m_CurrentRowHeight = vImage->getHeight();
    }
    m_LightmapImages.emplace_back(std::move(vImage));

    return LightmapIndex;
}

sptr<CIOImage> CLightmap::getCombinedLightmap()
{
    //return generateDiagonalGradientGrid(5290, 15, 255, 0, 0, 0, 255, 0);
    //return m_LightmapImages[0];
    size_t Resolution = m_TotalWidth * m_TotalHeight;
    uint8_t* pCombinedData = new uint8_t[Resolution * m_Channel];
    memset(pCombinedData, 0, Resolution * m_Channel);
    for (size_t i = 0; i < m_LightmapImages.size(); ++i)
    {
        sptr<CIOImage> pLightmap = m_LightmapImages[i];
        const uint8_t* pImageData = reinterpret_cast<const uint8_t*>(pLightmap->getData());

        auto [OffsetWidth, OffsetHeight] = m_Offsets[i];
        size_t RowNum = pLightmap->getHeight();
        size_t RowSize = m_Channel * pLightmap->getWidth();
        size_t Stroke = m_Channel * m_TotalWidth;
        size_t StartOffset = OffsetHeight * Stroke + OffsetWidth * m_Channel;
        for (size_t k = 0; k < RowNum; ++k)
        {
            memcpy_s(pCombinedData + StartOffset + k * Stroke, RowSize, pImageData + k * RowSize, RowSize);
        }
    }

    auto pCombinedLightmapImage = make<CIOImage>();
    pCombinedLightmapImage->setSize(m_TotalWidth, m_TotalHeight);
    pCombinedLightmapImage->setChannelNum(m_Channel);
    pCombinedLightmapImage->setData(pCombinedData);
    delete[] pCombinedData;

    return pCombinedLightmapImage;
}

std::pair<size_t, size_t> CLightmap::getLightmapSize()
{
    return std::make_pair(m_TotalWidth, m_TotalHeight);
}

glm::vec2 CLightmap::getAcutalLightmapCoord(size_t vIndex, glm::vec2 vOriginTexCoord)
{
    _ASSERTE(vIndex < m_LightmapImages.size());
    auto [OffsetWidth, OffsetHeight] = m_Offsets[vIndex];
    float OffsetX = static_cast<float>(OffsetWidth) / m_TotalWidth;
    float OffsetY = static_cast<float>(OffsetHeight) / m_TotalHeight;
    float ScaleWidth = static_cast<float>(m_LightmapImages[vIndex]->getWidth()) / m_TotalWidth;
    float ScaleHeight = static_cast<float>(m_LightmapImages[vIndex]->getHeight()) / m_TotalHeight;
    return glm::vec2(OffsetX + vOriginTexCoord.x * ScaleWidth, OffsetY + vOriginTexCoord.y * ScaleHeight);
}
