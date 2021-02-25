#include "Scene.h"
#include "SceneCommon.h"
#include "IOGoldSrcMap.h"
#include "IOGoldsrcWad.h"
#include "IOGoldSrcBsp.h"
#include "IOObj.h"

#include <filesystem>

S3DBoundingBox S3DObject::getBoundingBox() const
{
    std::optional<S3DBoundingBox> CachedBoundingBox = std::nullopt;
    if (CachedBoundingBox.has_value()) return CachedBoundingBox.value();
    S3DBoundingBox BoundingBox;
    BoundingBox.Min = glm::vec3(INFINITY, INFINITY, INFINITY);
    BoundingBox.Max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);
    for (size_t i = 0; i < Vertices.size(); ++i)
    {
        BoundingBox.Min.x = std::min<float>(BoundingBox.Min.x, Vertices[i].x);
        BoundingBox.Min.y = std::min<float>(BoundingBox.Min.y, Vertices[i].y);
        BoundingBox.Min.z = std::min<float>(BoundingBox.Min.z, Vertices[i].z);
        BoundingBox.Max.x = std::max<float>(BoundingBox.Max.x, Vertices[i].x);
        BoundingBox.Max.y = std::max<float>(BoundingBox.Max.y, Vertices[i].y);
        BoundingBox.Max.z = std::max<float>(BoundingBox.Max.z, Vertices[i].z);
    }
    CachedBoundingBox = BoundingBox;
    return BoundingBox;
}

bool SBspTreeNode::isPointFrontOfPlane(glm::vec3 vPoint) const
{
    return glm::dot(PlaneNormal, vPoint) - PlaneDistance > 0;
}

uint32_t SBspTree::getPointLeaf(glm::vec3 vPoint)
{
    if (Nodes.empty()) throw u8"场景无PVS数据";
    uint32_t NodeIndex = 0;
    while (NodeIndex < NodeNum)
    {
        if (Nodes[NodeIndex].isPointFrontOfPlane(vPoint))
        {
            NodeIndex = Nodes[NodeIndex].Front.value();
        }
        else
        {
            NodeIndex = Nodes[NodeIndex].Back.value();
        }
    }
    uint32_t LeafIndex = Nodes[NodeIndex].Index;

    return LeafIndex;
}

void SBspPvs::decompress(std::vector<uint8_t> vRawData, const SBspTree& vBspTree)
{
    RawData = vRawData;
    LeafNum = vBspTree.LeafNum;

    MapList.resize(LeafNum);
    MapList[0].resize(LeafNum, true); // leaf 0 can see all
    for (size_t i = 1; i < LeafNum; ++i)
    {
        std::optional<uint32_t> VisOffset = vBspTree.Nodes[vBspTree.NodeNum + i].PvsOffset;
        std::vector<uint8_t> DecompressedData;
        if (VisOffset.has_value())
            DecompressedData = __decompressFrom(VisOffset.value());

        MapList[i].resize(LeafNum);
        MapList[i][0] = false; // leaf 0 contains no data, and is always invisible
        for (size_t k = 1; k < LeafNum; ++k)
        {
            size_t ShiftIndex = k - 1; // vis data start at leaf 1, so need to shift index
            if (VisOffset.has_value())
            {
                uint8_t Byte = DecompressedData[ShiftIndex / 8];
                bool Visiable = (Byte & (1 << (ShiftIndex % 8)));
                MapList[i][k] = Visiable;
            }
            else
                MapList[i][k] = true;
        }
    }
}

bool SBspPvs::isLeafVisiable(uint32_t vStartLeafIndex, uint32_t vLeafIndex) const
{
    if (MapList.empty())
        return true;
    _ASSERTE(vStartLeafIndex < MapList.size());
    if (MapList[vStartLeafIndex].empty())
        return true;
    _ASSERTE(vLeafIndex < MapList[vStartLeafIndex].size());
    return MapList[vStartLeafIndex][vLeafIndex];
}

std::vector<uint8_t> SBspPvs::__decompressFrom(size_t vStartIndex)
{
    std::vector<uint8_t> DecompressedData;
    size_t RowLength = (LeafNum - 1 + 7) / 8;

    size_t Iter = vStartIndex;
    while (DecompressedData.size() < RowLength)
    {
        if (Iter >= RawData.size())
        {
            globalLog(u8"vis数据解码遇到末尾，已自动补零");
            while(DecompressedData.size() < RowLength)
                DecompressedData.emplace_back(0);
        }
        else if (RawData[Iter] > 0) // non-zero, just copy it
        {
            DecompressedData.emplace_back(RawData[Iter]);
            ++Iter;
        }
        else // zero, meaning the next byte tell how many zeros there are
        {
            ++Iter;
            _ASSERTE(Iter < RawData.size());
            uint8_t ZeroNum = RawData[Iter];
            while (ZeroNum > 0 && DecompressedData.size() < RowLength)
            {
                DecompressedData.emplace_back(0);
                ZeroNum--;
            }
            ++Iter;
        }
    }

    _ASSERTE(DecompressedData.size() == RowLength);
    return DecompressedData;
}

void CLightmap::clear()
{
    m_LightmapImages.clear();
    m_TotalWidth = m_TotalHeight = 0;
    m_StartHeight = m_StartWidth = m_CurrentRowHeight = 0;
    m_Offsets.clear();
}

size_t CLightmap::appendLightmap(std::shared_ptr<CIOImage> vpImage)
{
    if (m_Channel != vpImage->getImageChannels())
    {
        throw std::runtime_error(u8"错误：Lightmap图像的通道数量错误，要求为" + std::to_string(m_Channel) + u8"，实际为" + std::to_string(vpImage->getImageChannels()));
    }
    size_t LightmapIndex = m_LightmapImages.size();

    if (m_StartWidth + vpImage->getImageWidth() > m_MaxWidth) // next row
    {
        m_StartHeight += m_CurrentRowHeight;
        m_StartWidth = m_CurrentRowHeight = 0;
    }

    // simply place texture in a row
    m_Offsets.emplace_back(std::make_pair(m_StartWidth, m_StartHeight));
    m_StartWidth += vpImage->getImageWidth();
    m_TotalWidth = std::max<size_t>(m_TotalWidth, m_StartWidth);
    if (vpImage->getImageHeight() > m_CurrentRowHeight)
    {
        m_TotalHeight += vpImage->getImageHeight() - m_CurrentRowHeight;
        m_CurrentRowHeight = vpImage->getImageHeight();
    }
    m_LightmapImages.emplace_back(std::move(vpImage));

    return LightmapIndex;
}

std::shared_ptr<CIOImage> CLightmap::getCombinedLightmap()
{
    //return generateDiagonalGradientGrid(5290, 15, 255, 0, 0, 0, 255, 0);
    //return m_LightmapImages[0];
    size_t Resolution = m_TotalWidth * m_TotalHeight;
    uint8_t* pCombinedData = new uint8_t[Resolution * m_Channel];
    memset(pCombinedData, 0, Resolution * m_Channel);
    for (size_t i = 0; i < m_LightmapImages.size(); ++i)
    {
        std::shared_ptr<CIOImage> pLightmap = m_LightmapImages[i];
        const uint8_t* pImageData = reinterpret_cast<const uint8_t*>(pLightmap->getData());

        auto [OffsetWidth, OffsetHeight] = m_Offsets[i];
        size_t RowNum = pLightmap->getImageHeight();
        size_t RowSize = m_Channel * pLightmap->getImageWidth();
        size_t Stroke = m_Channel * m_TotalWidth;
        size_t StartOffset = OffsetHeight * Stroke + OffsetWidth * m_Channel;
        for (size_t k = 0; k < RowNum; ++k)
        {
            memcpy_s(pCombinedData + StartOffset + k * Stroke, RowSize, pImageData + k * RowSize, RowSize);
        }
    }

    auto pCombinedLightmapImage = std::make_shared<CIOImage>();
    pCombinedLightmapImage->setImageSize(m_TotalWidth, m_TotalHeight);
    pCombinedLightmapImage->setImageChannels(m_Channel);
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
    float ScaleWidth = static_cast<float>(m_LightmapImages[vIndex]->getImageWidth()) / m_TotalWidth;
    float ScaleHeight = static_cast<float>(m_LightmapImages[vIndex]->getImageHeight()) / m_TotalHeight;
    return glm::vec2(OffsetX + vOriginTexCoord.x * ScaleWidth, OffsetY + vOriginTexCoord.y * ScaleHeight);
}