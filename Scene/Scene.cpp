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
            GlobalLogger::logStream() << u8"vis数据解码遇到末尾，已自动补零";
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

std::shared_ptr<CIOImage> generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize)
{
    uint8_t BaseColor1[3] = { 0, 0, 0 };
    uint8_t BaseColor2[3] = { 255, 0, 255 };
    size_t DataSize = vNumRow * vNumCol * vCellSize * vCellSize * 4;
    uint8_t* pData = new uint8_t[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        size_t GridRowIndex = (i / (vNumCol * vCellSize)) / vCellSize;
        size_t GridColIndex = (i % (vNumCol * vCellSize)) / vCellSize;

        uint8_t* pColor;
        if ((GridRowIndex + GridColIndex) % 2 == 0)
            pColor = BaseColor1;
        else
            pColor = BaseColor2;
        pData[i * 4] = pColor[0];
        pData[i * 4 + 1] = pColor[1];
        pData[i * 4 + 2] = pColor[2];
        pData[i * 4 + 3] = static_cast<uint8_t>(255);
    }

    // cout 
    /*for (size_t i = 0; i < vNumRow * vCellSize; i++)
    {
        for (size_t k = 0; k < vNumCol * vCellSize; k++)
        {
            std::cout << pData[(i * vNumCol * vCellSize + k) * 4] % 2 << " ";
        }
        std::cout << std::endl;
    }*/
    std::shared_ptr<CIOImage> pGrid = std::make_shared<CIOImage>();
    pGrid->setImageSize(vNumCol * vCellSize, vNumRow * vCellSize);
    pGrid->setData(pData);

    return pGrid;
}

std::shared_ptr<CIOImage> generatePureColorTexture(uint8_t vBaseColor[3], size_t vSize)
{
    size_t DataSize = vSize * vSize * 4;
    uint8_t* pData = new uint8_t[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        pData[i * 4] = vBaseColor[0];
        pData[i * 4 + 1] = vBaseColor[1];
        pData[i * 4 + 2] = vBaseColor[2];
        pData[i * 4 + 3] = static_cast<uint8_t>(255);
    }

    std::shared_ptr<CIOImage> pGrid = std::make_shared<CIOImage>();
    pGrid->setImageSize(vSize, vSize);
    pGrid->setData(pData);

    return pGrid;
}

bool findFile(std::filesystem::path vFilePath, std::filesystem::path vSearchDir, std::filesystem::path& voFilePath)
{
    std::filesystem::path CurPath(vFilePath);

    std::filesystem::path FullPath = std::filesystem::absolute(vFilePath);
    std::filesystem::path CurDir = FullPath.parent_path();
    while (!CurDir.empty() && CurDir != CurDir.parent_path())
    {
        std::filesystem::path SearchPath = std::filesystem::relative(FullPath, CurDir);
        std::filesystem::path CombinedSearchPath = std::filesystem::path(vSearchDir) / SearchPath;
        if (std::filesystem::exists(SearchPath))
        {
            voFilePath = SearchPath;
            return true;
        }
        else if (std::filesystem::exists(CombinedSearchPath))
        {
            voFilePath = CombinedSearchPath;
            return true;
        }
        CurDir = CurDir.parent_path();
    }

    return false;
}

std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths, std::function<void(std::string)> vProgressReportFunc)
{
    std::vector<CIOGoldsrcWad> Wads(vWadPaths.size());

    for (size_t i = 0; i < vWadPaths.size(); ++i)
    {
        std::filesystem::path RealWadPath;
        if (!findFile(vWadPaths[i], "../data", RealWadPath))
            GlobalLogger::logStream() << u8"未找到或无法打开WAD文件：" << vWadPaths[i];
        if (vProgressReportFunc) vProgressReportFunc(u8"[wad]读取" + RealWadPath.u8string() + u8"文件中");
        Wads[i].read(RealWadPath);
    }
    return Wads;
}

std::shared_ptr<CIOImage> getIOImageFromWad(const CIOGoldsrcWad& vWad, size_t vIndex)
{
    uint32_t Width = 0, Height = 0;
    vWad.getTextureSize(vIndex, Width, Height);

    std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
    pTexImage->setImageSize(static_cast<int>(Width), static_cast<int>(Height));
    void* pData = new uint8_t[static_cast<size_t>(4) * Width * Height];
    vWad.getRawRGBAPixels(vIndex, pData);
    pTexImage->setData(pData);
    delete[] pData;
    return pTexImage;
}

std::vector<glm::vec3> getBspFaceVertices(const SBspLumps& vLumps, size_t vFaceIndex)
{
    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < vLumps.m_LumpFace.Faces.size());
    const SBspFace& Face = vLumps.m_LumpFace.Faces[vFaceIndex];
    _ASSERTE(static_cast<size_t>(Face.FirstSurfedgeIndex) + Face.NumSurfedge <= vLumps.m_LumpSurfedge.Surfedges.size());

    // extract face vertex index from edges
    std::vector<uint16_t> FaceVertexIndices;
    uint16_t LastVertexIndex2 = 0;
    for (uint16_t i = 0; i < Face.NumSurfedge; ++i)
    {
        size_t SurfedgeIndex = static_cast<size_t>(Face.FirstSurfedgeIndex) + i;
        int32_t RawEdgeIndex = vLumps.m_LumpSurfedge.Surfedges[SurfedgeIndex];
        uint16_t VertexIndex1 = 0, VertexIndex2 = 0;
        if (RawEdgeIndex > 0)
        {
            uint32_t EdgeIndex = static_cast<uint32_t>(RawEdgeIndex);
            _ASSERTE(EdgeIndex < vLumps.m_LumpEdge.Edges.size());
            VertexIndex1 = vLumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[0];
            VertexIndex2 = vLumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[1];
        }
        else
        {
            uint32_t EdgeIndex = static_cast<uint32_t>(-static_cast<int64_t>(RawEdgeIndex));
            _ASSERTE(EdgeIndex < vLumps.m_LumpEdge.Edges.size());
            VertexIndex1 = vLumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[1];
            VertexIndex2 = vLumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[0];
        }

        FaceVertexIndices.emplace_back(VertexIndex1);
        if ((i > 0 && LastVertexIndex2 != VertexIndex1) ||
            (i == Face.NumSurfedge - 1 && FaceVertexIndices[0] != VertexIndex2))
        {
            throw std::runtime_error(u8"BSP文件中，面的边未组成环");
        }
        LastVertexIndex2 = VertexIndex2;
    }

    // get face vertices from indices
    std::vector<glm::vec3> FaceVertices;
    for (uint16_t FaceVertexIndex : FaceVertexIndices)
    {
        _ASSERTE(FaceVertexIndex < vLumps.m_LumpVertex.Vertices.size());
        SVec3 Vertex = vLumps.m_LumpVertex.Vertices[FaceVertexIndex];
        FaceVertices.emplace_back(Vertex.glmVec3());
    }

    return FaceVertices;
}

glm::vec3 getBspFaceNormal(const SBspLumps& vLumps, size_t vFaceIndex)
{
    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < vLumps.m_LumpFace.Faces.size());
    const SBspFace& Face = vLumps.m_LumpFace.Faces[vFaceIndex];
    // get plane
    _ASSERTE(Face.PlaneIndex < vLumps.m_LumpPlane.Planes.size());
    const SBspPlane& Plane = vLumps.m_LumpPlane.Planes[Face.PlaneIndex];
    bool ReverseNormal = (Face.PlaneSide != 0);
    glm::vec3 Normal = Plane.Normal.glmVec3();
    if (ReverseNormal) Normal *= -1;
    return Normal;
}

std::vector<glm::vec2> getBspFaceUnnormalizedTexCoords(const SBspLumps& vLumps, size_t vFaceIndex, std::vector<glm::vec3> vVertices)
{
    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < vLumps.m_LumpFace.Faces.size());
    const SBspFace& Face = vLumps.m_LumpFace.Faces[vFaceIndex];

    // find texture size
    uint16_t TexInfoIndex = Face.TexInfoIndex;
    _ASSERTE(TexInfoIndex < vLumps.m_LumpTexInfo.TexInfos.size());
    const SBspTexInfo& TexInfo = vLumps.m_LumpTexInfo.TexInfos[TexInfoIndex];

    std::vector<glm::vec2> TexCoords;
    for (const glm::vec3& Vertex : vVertices)
        TexCoords.emplace_back(TexInfo.getTexCoord(Vertex));
    return TexCoords;
}

// vTexCoords should contain unnormalized texcoords to avoid known of texture width and height
void getBspFaceLightmap(const SBspLumps& vLumps, size_t vFaceIndex, const std::vector<glm::vec2>& vTexCoords, std::vector<glm::vec2>& voLightmapCoords, std::shared_ptr<CIOImage>& vopLightmapImage)
{
    voLightmapCoords.clear();
    vopLightmapImage = nullptr;

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < vLumps.m_LumpFace.Faces.size());
    const SBspFace& Face = vLumps.m_LumpFace.Faces[vFaceIndex];

    const float LightmapScale = 16.0f; // It should be 16.0 in GoldSrc. BTW, Source engine VHE seems to be able to change this.
    // TODO: handle lighting style like sky, no-draw, etc.
    if (vLumps.m_LumpLighting.Lightmaps.size() > 0 && Face.LightmapOffset < std::numeric_limits<uint32_t>::max())
    {
        glm::vec2 ScaledLightmapBoundMin = { INFINITY, INFINITY };
        glm::vec2 ScaledLightmapBoundMax = { -INFINITY, -INFINITY };
        for (const glm::vec2& TexCoord : vTexCoords)
        {
            ScaledLightmapBoundMin.x = std::min<float>(ScaledLightmapBoundMin.x, TexCoord.x);
            ScaledLightmapBoundMin.y = std::min<float>(ScaledLightmapBoundMin.y, TexCoord.y);
            ScaledLightmapBoundMax.x = std::max<float>(ScaledLightmapBoundMax.x, TexCoord.x);
            ScaledLightmapBoundMax.y = std::max<float>(ScaledLightmapBoundMax.y, TexCoord.y);
        }
        ScaledLightmapBoundMin.x = ScaledLightmapBoundMin.x / LightmapScale;
        ScaledLightmapBoundMin.y = ScaledLightmapBoundMin.y / LightmapScale;
        ScaledLightmapBoundMax.x = ScaledLightmapBoundMax.x / LightmapScale;
        ScaledLightmapBoundMax.y = ScaledLightmapBoundMax.y / LightmapScale;

        // From https://github.com/Sergey-KoRJiK/GldSrcBSPditor says:
        // Lightmap samples stored in corner of samples, instead center of samples
        // so lightmap size need increment by one
        int MinX = static_cast<int>(std::floor(ScaledLightmapBoundMin.x));
        int MinY = static_cast<int>(std::floor(ScaledLightmapBoundMin.y));
        int MaxX = std::max<int>(static_cast<int>(std::ceil(ScaledLightmapBoundMax.x)), MinX);
        int MaxY = std::max<int>(static_cast<int>(std::ceil(ScaledLightmapBoundMax.y)), MinY);

        size_t LightmapWidth = static_cast<size_t>(MaxX - MinX) + 1;
        size_t LightmapHeight = static_cast<size_t>(MaxY - MinY) + 1;

        size_t LightmapImageSize = static_cast<size_t>(4) * LightmapWidth * LightmapHeight;
        uint8_t* pData = new uint8_t[LightmapImageSize];
        std::memset(pData, 0, LightmapImageSize);
        uint8_t* pTempData = new uint8_t[LightmapImageSize];
        // blend all lightmap, use the brightest value
        for (int i = 0; i < 4; ++i)
        {
            if (Face.LightingStyles[i] == 0xff) continue;
            vLumps.m_LumpLighting.getRawRGBAPixels(Face.LightmapOffset / 3 + LightmapImageSize / 4 * i, LightmapImageSize / 4, pTempData);
            for (size_t k = 0; k < LightmapImageSize; ++k)
                pData[k] = std::max<uint8_t>(pData[k], pTempData[k]);
        }
        vopLightmapImage = std::make_shared<CIOImage>();
        vopLightmapImage->setImageSize(LightmapWidth, LightmapHeight);
        vopLightmapImage->setData(pData);
        delete[] pTempData;
        delete[] pData;

        for (const glm::vec2& TexCoord : vTexCoords)
        {
            glm::vec2 LightmapCoord = TexCoord;
            LightmapCoord.x /= LightmapScale;
            LightmapCoord.x -= MinX;
            LightmapCoord.x += 0.5f;
            LightmapCoord.x /= LightmapWidth;

            LightmapCoord.y /= LightmapScale;
            LightmapCoord.y -= MinY;
            LightmapCoord.y += 0.5f;
            LightmapCoord.y /= LightmapHeight;

            voLightmapCoords.emplace_back(LightmapCoord);
        }
    }
}

void getBspFaceTextureSizeAndName(const SBspLumps& vLumps, size_t vFaceIndex, size_t& voWidth, size_t& voHeight, std::string& voName)
{
    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < vLumps.m_LumpFace.Faces.size());
    const SBspFace& Face = vLumps.m_LumpFace.Faces[vFaceIndex];

    uint16_t TexInfoIndex = Face.TexInfoIndex;
    _ASSERTE(TexInfoIndex < vLumps.m_LumpTexInfo.TexInfos.size());
    const SBspTexInfo& TexInfo = vLumps.m_LumpTexInfo.TexInfos[TexInfoIndex];
    _ASSERTE(TexInfo.TextureIndex < vLumps.m_LumpTexture.Textures.size());
    const SBspTexture& BspTexture = vLumps.m_LumpTexture.Textures[TexInfo.TextureIndex];

    voWidth = BspTexture.Width;
    voHeight = BspTexture.Height;
    voName = BspTexture.Name;
}

SScene SceneReader::readBspFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    const float SceneScale = 1.0f / 64.0f;
    if (vProgressReportFunc) vProgressReportFunc(u8"[bsp]读取文件中");
    CIOGoldSrcBsp Bsp = CIOGoldSrcBsp(vFilePath);
    if (!Bsp.read())
        throw std::runtime_error(u8"文件解析失败");
    SScene Scene;
    Scene.UseLightmap = true;

    const SBspLumps& Lumps = Bsp.getLumps();

    // read wads
    const std::vector<std::filesystem::path>& WadPaths = Lumps.m_LumpEntity.WadPaths;
    std::vector<CIOGoldsrcWad> Wads = readWads(WadPaths, vProgressReportFunc);

    // load textures
    // iterate each texture in texture lump
    // if bsp contains its data, load it, otherwise find it in all wads
    // if found, load it, otherwise set mapper to 0
    if (vProgressReportFunc) vProgressReportFunc(u8"整理纹理中");
    std::map<std::string, uint32_t> TexNameToIndex;
    Scene.TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    TexNameToIndex["TextureNotFound"] = 0;
    for (size_t i = 0; i < Lumps.m_LumpTexture.Textures.size(); ++i)
    {
        const SBspTexture& BspTexture = Lumps.m_LumpTexture.Textures[i];
        if (vProgressReportFunc) vProgressReportFunc(u8"读取纹理（" + std::to_string(i + 1) + "/" + std::to_string(Lumps.m_LumpTexture.Textures.size()) + " " + BspTexture.Name + u8"）");
        if (BspTexture.IsDataInBsp)
        {
            uint8_t* pData = new uint8_t[static_cast<size_t>(4) * BspTexture.Width * BspTexture.Height];
            BspTexture.getRawRGBAPixels(pData);
            std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
            pTexImage->setImageSize(BspTexture.Width, BspTexture.Height);
            pTexImage->setData(pData);
            delete[] pData;
            TexNameToIndex[BspTexture.Name] = Scene.TexImages.size();
            Scene.TexImages.emplace_back(std::move(pTexImage));
        }
        else
        {
            bool Found = false;
            for (const CIOGoldsrcWad& Wad : Wads)
            {
                std::optional<size_t> Index = Wad.findTexture(BspTexture.Name);
                if (Index.has_value())
                {
                    Found = true;
                    TexNameToIndex[BspTexture.Name] = Scene.TexImages.size();

                    std::shared_ptr<CIOImage> pTexImage = getIOImageFromWad(Wad, Index.value());
                    Scene.TexImages.emplace_back(std::move(pTexImage));
                    break;
                }
            }
            if (!Found)
                TexNameToIndex[BspTexture.Name] = 0;
        }
    }

    // load leaves, one object for each leaf
    if (vProgressReportFunc) vProgressReportFunc(u8"载入场景数据");
    for (const SBspLeaf& Leaf : Lumps.m_LumpLeaf.Leaves)
    {
        auto pObject = std::make_shared<S3DObject>();
        pObject->UseShadow = false;
        for (uint16_t i = 0; i < Leaf.NumMarkSurface; ++i)
        {
            uint16_t MarkSurfaceIndex = Leaf.FirstMarkSurfaceIndex + i;
            _ASSERTE(MarkSurfaceIndex < Lumps.m_LumpMarkSurface.FaceIndices.size());
            uint16_t FaceIndex = Lumps.m_LumpMarkSurface.FaceIndices[MarkSurfaceIndex];
            size_t TexWidth, TexHeight;
            std::string TexName;
            getBspFaceTextureSizeAndName(Lumps, FaceIndex, TexWidth, TexHeight, TexName);
            std::vector<glm::vec3> Vertices = getBspFaceVertices(Lumps, FaceIndex);
            glm::vec3 Normal = getBspFaceNormal(Lumps, FaceIndex);
            std::vector<glm::vec2> TexCoords = getBspFaceUnnormalizedTexCoords(Lumps, FaceIndex, Vertices);
            std::vector<glm::vec2> LightmapCoords;
            std::shared_ptr<CIOImage> pLightmapImage = nullptr;
            getBspFaceLightmap(Lumps, FaceIndex, TexCoords, LightmapCoords, pLightmapImage);

            uint32_t TexIndex = TexNameToIndex[TexName];
            uint32_t LightmapIndex = std::numeric_limits<uint32_t>::max();
            if (pLightmapImage)
            {
                LightmapIndex = Scene.LightmapImages.size();
                Scene.LightmapImages.emplace_back(pLightmapImage);
            }

            // scale texture coordinates
            for (glm::vec2& TexCoord : TexCoords)
            {
                TexCoord.x /= TexWidth;
                TexCoord.y /= TexHeight;
            }

            // save scaled datas
            for (size_t k = 2; k < Vertices.size(); ++k)
            {
                pObject->Vertices.emplace_back(Vertices[0] * SceneScale);
                pObject->Vertices.emplace_back(Vertices[k - 1] * SceneScale);
                pObject->Vertices.emplace_back(Vertices[k] * SceneScale);
                pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
                pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
                pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
                pObject->Normals.emplace_back(Normal);
                pObject->Normals.emplace_back(Normal);
                pObject->Normals.emplace_back(Normal);
                pObject->TexCoords.emplace_back(TexCoords[0]);
                pObject->TexCoords.emplace_back(TexCoords[k - 1]);
                pObject->TexCoords.emplace_back(TexCoords[k]);
                if (LightmapIndex != std::numeric_limits<uint32_t>::max())
                {
                    pObject->LightmapCoords.emplace_back(LightmapCoords[0]);
                    pObject->LightmapCoords.emplace_back(LightmapCoords[k - 1]);
                    pObject->LightmapCoords.emplace_back(LightmapCoords[k]);
                }
                else
                {
                    pObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
                    pObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
                    pObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
                }

                pObject->TexIndices.emplace_back(TexIndex);
                pObject->TexIndices.emplace_back(TexIndex);
                pObject->TexIndices.emplace_back(TexIndex);
                pObject->LightmapIndices.emplace_back(LightmapIndex);
                pObject->LightmapIndices.emplace_back(LightmapIndex);
                pObject->LightmapIndices.emplace_back(LightmapIndex);
            }
        }
        Scene.Objects.emplace_back(std::move(pObject));
    }

    // read node and PVS data
    if (vProgressReportFunc) vProgressReportFunc(u8"载入BSP与VIS数据");
    Scene.UsePVS = true;
    size_t NodeNum = Lumps.m_LumpNode.Nodes.size();
    size_t LeafNum = Lumps.m_LumpLeaf.Leaves.size();

    Scene.BspTree.NodeNum = NodeNum;
    Scene.BspTree.LeafNum = LeafNum;
    Scene.BspTree.Nodes.resize(NodeNum + LeafNum);
    for (size_t i = 0; i < NodeNum; ++i)
    {
        const SBspNode& OriginNode = Lumps.m_LumpNode.Nodes[i];
        _ASSERTE(OriginNode.PlaneIndex < Lumps.m_LumpPlane.Planes.size());
        const SBspPlane& OriginPlane = Lumps.m_LumpPlane.Planes[OriginNode.PlaneIndex];
        SBspTreeNode& Node = Scene.BspTree.Nodes[i];

        Node.Index = i;
        Node.PlaneNormal = OriginPlane.Normal.glmVec3();
        Node.PlaneDistance = OriginPlane.DistanceToOrigin * SceneScale;

        if (OriginNode.ChildrenIndices[0] > 0)
            Node.Front = OriginNode.ChildrenIndices[0];
        else
            Node.Front = ~OriginNode.ChildrenIndices[0] + NodeNum;
        if (OriginNode.ChildrenIndices[1] > 0)
            Node.Back = OriginNode.ChildrenIndices[1];
        else
            Node.Back = ~OriginNode.ChildrenIndices[1] + NodeNum;
    }
    for (size_t i = 0; i < LeafNum; ++i)
    {
        const SBspLeaf& OriginLeaf = Lumps.m_LumpLeaf.Leaves[i];
        SBspTreeNode& Node = Scene.BspTree.Nodes[NodeNum + i];

        Node.Index = i;
        if (OriginLeaf.VisOffset >= 0)
            Node.PvsOffset = OriginLeaf.VisOffset;
    }
    Scene.BspPvs.decompress(Lumps.m_LumpVisibility.Vis, Scene.BspTree);
    
    return Scene;
}

SScene SceneReader::readMapFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    if (vProgressReportFunc) vProgressReportFunc(u8"[map]读取文件中");
    CIOGoldSrcMap Map = CIOGoldSrcMap(vFilePath);
    if (!Map.read())
        throw std::runtime_error(u8"文件解析失败");
    std::vector<std::filesystem::path> WadPaths = Map.getWadPaths();
    std::vector<CIOGoldsrcWad> Wads = readWads(WadPaths, vProgressReportFunc);

    SScene Scene;

    if (vProgressReportFunc) vProgressReportFunc(u8"整理纹理中");
    // find used textures, load and index them
    std::map<std::string, uint32_t> TexNameToIndex;
    std::set<std::string> UsedTextureNames = Map.getUsedTextureNames();
    Scene.TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    TexNameToIndex["TextureNotFound"] = 0;
    UsedTextureNames.insert("TextureNotFound");
    for (const std::string& TexName : UsedTextureNames)
    {
        bool Found = false;
        for (const CIOGoldsrcWad& Wad : Wads)
        {
            std::optional<size_t> Index = Wad.findTexture(TexName);
            if (Index.has_value())
            {
                Found = true;
                TexNameToIndex[TexName] = Scene.TexImages.size();

                std::shared_ptr<CIOImage> pTexImage = getIOImageFromWad(Wad, Index.value());
                Scene.TexImages.emplace_back(std::move(pTexImage));
                break;
            }
        }
        if (!Found)
            TexNameToIndex[TexName] = 0;
    }

    if (vProgressReportFunc) vProgressReportFunc(u8"生成场景中");
    // group polygon by texture, one object per texture 
    Scene.Objects.resize(UsedTextureNames.size());
    for (size_t i = 0; i < Scene.Objects.size(); ++i)
    {
        Scene.Objects[i] = std::make_shared<S3DObject>();
    }

    std::vector<SMapPolygon> Polygons = Map.getAllPolygons();

    for (SMapPolygon& Polygon : Polygons)
    {
        size_t TexIndex = TexNameToIndex[Polygon.pPlane->TextureName];
        size_t TexWidth = Scene.TexImages[TexIndex]->getImageWidth();
        size_t TexHeight = Scene.TexImages[TexIndex]->getImageHeight();
        std::shared_ptr<S3DObject> pObject = Scene.Objects[TexIndex];
        uint32_t IndexStart = pObject->Vertices.size();

        std::vector<glm::vec2> TexCoords = Polygon.getTexCoords(TexWidth, TexHeight);
        glm::vec3 Normal = Polygon.getNormal();
        const uint32_t LightmapIndex = std::numeric_limits<uint32_t>::max();

        // indexed data
        /*Object.Vertices.insert(Object.Vertices.end(), Polygon.Vertices.begin(), Polygon.Vertices.end());
        Object.TexCoords.insert(Object.TexCoords.end(), TexCoords.begin(), TexCoords.end());
        for (size_t i = 0; i < Polygon.Vertices.size(); ++i)
        {
            Object.Colors.emplace_back(glm::vec3(1.0, 0.0, 0.0));
            Object.Normals.emplace_back(Normal);
        }

        for (size_t i = 2; i < Polygon.Vertices.size(); ++i)
        {
            Object.Indices.emplace_back(IndexStart);
            Object.Indices.emplace_back(IndexStart + i - 1);
            Object.Indices.emplace_back(IndexStart + i);
        }
        IndexStart += static_cast<uint32_t>(Polygon.Vertices.size());*/

        // non-indexed data
        for (size_t k = 2; k < Polygon.Vertices.size(); ++k)
        {
            pObject->Vertices.emplace_back(Polygon.Vertices[0]);
            pObject->Vertices.emplace_back(Polygon.Vertices[k - 1]);
            pObject->Vertices.emplace_back(Polygon.Vertices[k]);
            pObject->Normals.emplace_back(Normal);
            pObject->Normals.emplace_back(Normal);
            pObject->Normals.emplace_back(Normal);
            pObject->TexCoords.emplace_back(TexCoords[0]);
            pObject->TexCoords.emplace_back(TexCoords[k - 1]);
            pObject->TexCoords.emplace_back(TexCoords[k]);
            pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObject->TexIndices.emplace_back(TexIndex);
            pObject->TexIndices.emplace_back(TexIndex);
            pObject->TexIndices.emplace_back(TexIndex);
            pObject->LightmapIndices.emplace_back(LightmapIndex);
            pObject->LightmapIndices.emplace_back(LightmapIndex);
            pObject->LightmapIndices.emplace_back(LightmapIndex);
        }
    }
    if (vProgressReportFunc) vProgressReportFunc(u8"完成");
    return Scene;
}

SScene SceneReader::readObjFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    if (vProgressReportFunc) vProgressReportFunc(u8"[obj]读取文件中");
    CIOObj Obj = CIOObj();
    Obj.read(vFilePath);

    if (vProgressReportFunc) vProgressReportFunc(u8"生成场景中");
    std::shared_ptr<S3DObject> pObjObject = std::make_shared<S3DObject>();
    const uint32_t TexIndex = 0;
    const uint32_t LightmapIndex = std::numeric_limits<uint32_t>::max();

    const std::vector<SObjFace>& Faces = Obj.getFaces();
    for (size_t i = 0; i < Faces.size(); ++i)
    {
        const SObjFace& Face = Faces[i];
        for (size_t k = 2; k < Face.Nodes.size(); ++k)
        {
            pObjObject->Vertices.emplace_back(Obj.getVertex(i, 0));
            pObjObject->Vertices.emplace_back(Obj.getVertex(i, k - 1));
            pObjObject->Vertices.emplace_back(Obj.getVertex(i, k));
            pObjObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObjObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObjObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObjObject->Normals.emplace_back(Obj.getNormal(i, 0));
            pObjObject->Normals.emplace_back(Obj.getNormal(i, k - 1));
            pObjObject->Normals.emplace_back(Obj.getNormal(i, k));
            pObjObject->TexCoords.emplace_back(Obj.getTexCoord(i, 0));
            pObjObject->TexCoords.emplace_back(Obj.getTexCoord(i, k - 1));
            pObjObject->TexCoords.emplace_back(Obj.getTexCoord(i, k));
            pObjObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObjObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObjObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObjObject->TexIndices.emplace_back(TexIndex);
            pObjObject->TexIndices.emplace_back(TexIndex);
            pObjObject->TexIndices.emplace_back(TexIndex);
            pObjObject->LightmapIndices.emplace_back(LightmapIndex);
            pObjObject->LightmapIndices.emplace_back(LightmapIndex);
            pObjObject->LightmapIndices.emplace_back(LightmapIndex);
        }
    }

    SScene Scene;
    Scene.Objects.emplace_back(pObjObject);
    Scene.TexImages.emplace_back(generateBlackPurpleGrid(4, 4, 16));
    //CIOImage Texture("../data/Tex2.png");
    //Texture.read();
    //TexImages.push_back(Texture);

    return Scene;
}