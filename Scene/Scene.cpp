#include "Scene.h"

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

bool SBspPvs::isVisiableLeafVisiable(uint32_t vStartLeafIndex, uint32_t vLeafIndex) const
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
        // TODO: seems the RawData length is shorter than espected
        // when encounter end of RawData and the decompress is not finished, fill zero?
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