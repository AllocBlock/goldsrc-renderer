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

size_t CIOGoldSrcSpr::getFrameTime(size_t vIndex) const
{
    _ASSERTE(vIndex < m_TimeSet.size());
    return m_TimeSet[vIndex];
}

void CIOGoldSrcSpr::getFrameRGBAPixels(size_t vIndex, void* voData) const
{
    _ASSERTE(vIndex < m_TimeSet.size());
    if (m_Header.Version == 0x100000)
    {
        memcpy(voData, m_FrameSet[vIndex].DataSet.data(), m_FrameSet[vIndex].DataSet.size());
    }
    else if (m_Header.Version == 0x01)
    {
        throw std::runtime_error(u8"索引数据，无调色板信息，无法获取图像数据");
    }
    else
    {
        throw std::runtime_error(u8"存储类型错误");
    }
}

bool CIOGoldSrcSpr::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File(vFilePath, std::ios::in | std::ios::binary);
    if (!File.is_open())
        throw std::runtime_error(u8"文件读取失败");

    // 读取头
    File.read(reinterpret_cast<char*>(&m_Header), sizeof(SSprHeader));
    _ASSERTE(m_Header.Magic[0] == "I" &&
        m_Header.Magic[0] == "D" &&
        m_Header.Magic[0] == "S" &&
        m_Header.Magic[0] == "P");

    int32_t FrameSetTypeMark = 0;
    File.read(reinterpret_cast<char*>(&FrameSetTypeMark), sizeof(int32_t));
    if (FrameSetTypeMark == 0x00) // 仅一帧
    {
        m_FrameSet.emplace_back(readPicture(File));
    }
    else if (FrameSetTypeMark == 0x01 || FrameSetTypeMark == 0x10000000) // 多帧
    {
        int32_t FrameNum = 0;
        File.read(reinterpret_cast<char*>(&FrameNum), sizeof(int32_t));
        m_TimeSet.resize(FrameNum, 0);
        File.read(reinterpret_cast<char*>(m_TimeSet.data()), sizeof(float) * FrameNum);
        for(int32_t i = 0; i < FrameNum; ++i)
            m_FrameSet.emplace_back(readPicture(File));
    }
    else
    {
        throw std::runtime_error(u8"帧类型错误");
    }
}

SSprPicture CIOGoldSrcSpr::readPicture(std::ifstream& voFile)
{
    SSprPicture Picture;
    voFile.read(reinterpret_cast<char*>(&Picture), sizeof(int32_t) * 4);
    if (m_Header.Version == 0x01) // 索引存储（Quake，调色板为游戏全局调色板）
    {
        size_t Size = Picture.Width * Picture.Height;
        Picture.DataSet.resize(Size);
        voFile.read(reinterpret_cast<char*>(Picture.DataSet.data()), sizeof(uint8_t) * Size);
    }
    else if (m_Header.Version == 0x100000) // 32位直接存储
    {
        size_t Size = Picture.Width * Picture.Height * 4;
        Picture.DataSet.resize(Size);
        voFile.read(reinterpret_cast<char*>(Picture.DataSet.data()), sizeof(uint8_t) * Size);
    }
    else
    {
        throw std::runtime_error(u8"存储类型错误");
    }
}