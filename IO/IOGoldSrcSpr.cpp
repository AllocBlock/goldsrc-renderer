#include "IOGoldSrcSpr.h"

size_t CIOGoldSrcSpr::getFrameNum() const
{
    return m_FrameSet.size();
}

void CIOGoldSrcSpr::getFrameSize(size_t vIndex, uint32_t& voWidth, uint32_t& voHeight) const
{
    _ASSERTE(vIndex < m_FrameSet.size());
    voWidth = m_FrameSet[vIndex].Width;
    voHeight = m_FrameSet[vIndex].Height;
}

void CIOGoldSrcSpr::getFrameRGBAPixels(size_t vIndex, void* voData) const
{
    _ASSERTE(vIndex < m_FrameSet.size());
    size_t Size = m_FrameSet[vIndex].Width * m_FrameSet[vIndex].Height;
    uint8_t* pPtr = static_cast<uint8_t*>(voData);
    for (size_t i = 0; i < Size; ++i)
    {
        auto Index = m_FrameSet[vIndex].IndexSet[i];
        auto Color = m_Palatte.ColorSet[Index];
        pPtr[i * 4] = Color.R;
        pPtr[i * 4 + 1] = Color.G;
        pPtr[i * 4 + 2] = Color.B;
        pPtr[i * 4 + 3] = 0xff;
    }
}

bool CIOGoldSrcSpr::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File(vFilePath, std::ios::in | std::ios::binary);
    if (!File.is_open())
        throw std::runtime_error(u8"文件读取失败");

    // 读取头
    File.read(reinterpret_cast<char*>(&m_Header), sizeof(SSprHeader));
    _ASSERTE(m_Header.Magic[0] == 'I' &&
        m_Header.Magic[1] == 'D' &&
        m_Header.Magic[2] == 'S' &&
        m_Header.Magic[3] == 'P');

    // 读取调色板
    File.read(reinterpret_cast<char*>(&m_Palatte), sizeof(SSprPalette));
    _ASSERTE(m_Palatte.Size == 256);

    // 读取帧
    m_FrameSet.resize(m_Header.FrameNum);
    for (int32_t i = 0; i < m_Header.FrameNum; ++i)
        readPicture(File, m_FrameSet[i]);
}

void CIOGoldSrcSpr::readPicture(std::ifstream& voFile, SSprPicture& voPicture)
{
    voFile.read(reinterpret_cast<char*>(&voPicture), sizeof(int32_t) * 5);
    size_t Size = voPicture.Width * voPicture.Height;
    voPicture.IndexSet.resize(Size);
    voFile.read(reinterpret_cast<char*>(voPicture.IndexSet.data()), sizeof(uint8_t) * Size);
}