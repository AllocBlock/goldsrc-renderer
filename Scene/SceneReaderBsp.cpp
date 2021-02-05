#include "SceneReaderBsp.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "IOGoldSrcWad.h"

SScene CSceneReaderBsp::read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    m_ProgressReportFunc = vProgressReportFunc;
    m_Scene = SScene();
    m_Scene.UseLightmap = true;

    __readBsp(vFilePath);
    __readTextures();
    __loadLeaves();
    __loadEntities();
    __loadBspTreeAndPvs();

    return m_Scene;
}

void CSceneReaderBsp::__readBsp(std::filesystem::path vFilePath)
{
    __reportProgress(u8"[bsp]读取文件中");
    m_Bsp = CIOGoldSrcBsp(vFilePath);
    if (!m_Bsp.read())
        throw std::runtime_error(u8"文件解析失败");
}

void CSceneReaderBsp::__readTextures()
{
    m_TexNameToIndex.clear();
    const SBspLumps& Lumps = m_Bsp.getLumps();

    // read wads
    const std::vector<std::filesystem::path>& WadPaths = Lumps.m_LumpEntity.WadPaths;
    std::vector<CIOGoldsrcWad> Wads = readWads(WadPaths, m_ProgressReportFunc);

    // load textures
    // iterate each texture in texture lump
    // if bsp contains its data, load it, otherwise find it in all wads
    // if found, load it, otherwise set mapper to 0
    __reportProgress(u8"整理纹理中");
    m_Scene.TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    m_TexNameToIndex["TextureNotFound"] = 0;
    for (size_t i = 0; i < Lumps.m_LumpTexture.Textures.size(); ++i)
    {
        const SBspTexture& BspTexture = Lumps.m_LumpTexture.Textures[i];
        __reportProgress(u8"读取纹理（" + std::to_string(i + 1) + "/" + std::to_string(Lumps.m_LumpTexture.Textures.size()) + " " + BspTexture.Name + u8"）");
        if (BspTexture.IsDataInBsp)
        {
            std::shared_ptr<CIOImage> pTexImage = getIOImageFromBspTexture(BspTexture);
            m_TexNameToIndex[BspTexture.Name] = m_Scene.TexImages.size();
            m_Scene.TexImages.emplace_back(std::move(pTexImage));
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
                    std::shared_ptr<CIOImage> pTexImage = getIOImageFromWad(Wad, Index.value());
                    m_TexNameToIndex[BspTexture.Name] = m_Scene.TexImages.size();
                    m_Scene.TexImages.emplace_back(std::move(pTexImage));
                    break;
                }
            }
            if (!Found)
                m_TexNameToIndex[BspTexture.Name] = 0;
        }
    }
}

void CSceneReaderBsp::__loadLeaves()
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    // load leaves, one object for each leaf
    __reportProgress(u8"载入场景数据");
    for (const SBspLeaf& Leaf : Lumps.m_LumpLeaf.Leaves)
    {
        auto pObject = std::make_shared<S3DObject>();
        pObject->UseShadow = false;
        for (uint16_t i = 0; i < Leaf.NumMarkSurface; ++i)
        {
            uint16_t MarkSurfaceIndex = Leaf.FirstMarkSurfaceIndex + i;
            _ASSERTE(MarkSurfaceIndex < Lumps.m_LumpMarkSurface.FaceIndices.size());
            uint16_t FaceIndex = Lumps.m_LumpMarkSurface.FaceIndices[MarkSurfaceIndex];
            __appendBspFaceToObject(pObject, FaceIndex);
        }
        m_Scene.Objects.emplace_back(std::move(pObject));
    }
}

void CSceneReaderBsp::__loadEntities()
{
    
}

void CSceneReaderBsp::__loadBspTreeAndPvs()
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    // read node and PVS data
    __reportProgress(u8"载入BSP与VIS数据");
    m_Scene.UsePVS = true;
    size_t NodeNum = Lumps.m_LumpNode.Nodes.size();
    size_t LeafNum = Lumps.m_LumpLeaf.Leaves.size();

    m_Scene.BspTree.NodeNum = NodeNum;
    m_Scene.BspTree.LeafNum = LeafNum;
    m_Scene.BspTree.Nodes.resize(NodeNum + LeafNum);
    for (size_t i = 0; i < NodeNum; ++i)
    {
        const SBspNode& OriginNode = Lumps.m_LumpNode.Nodes[i];
        _ASSERTE(OriginNode.PlaneIndex < Lumps.m_LumpPlane.Planes.size());
        const SBspPlane& OriginPlane = Lumps.m_LumpPlane.Planes[OriginNode.PlaneIndex];
        SBspTreeNode& Node = m_Scene.BspTree.Nodes[i];

        Node.Index = i;
        Node.PlaneNormal = OriginPlane.Normal.glmVec3();
        Node.PlaneDistance = OriginPlane.DistanceToOrigin * m_SceneScale;

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
        SBspTreeNode& Node = m_Scene.BspTree.Nodes[NodeNum + i];

        Node.Index = i;
        if (OriginLeaf.VisOffset >= 0)
            Node.PvsOffset = OriginLeaf.VisOffset;
    }
    m_Scene.BspPvs.decompress(Lumps.m_LumpVisibility.Vis, m_Scene.BspTree);
}

std::vector<glm::vec3> CSceneReaderBsp::__getBspFaceVertices(size_t vFaceIndex)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < Lumps.m_LumpFace.Faces.size());
    const SBspFace& Face = Lumps.m_LumpFace.Faces[vFaceIndex];
    _ASSERTE(static_cast<size_t>(Face.FirstSurfedgeIndex) + Face.NumSurfedge <= Lumps.m_LumpSurfedge.Surfedges.size());

    // extract face vertex index from edges
    std::vector<uint16_t> FaceVertexIndices;
    uint16_t LastVertexIndex2 = 0;
    for (uint16_t i = 0; i < Face.NumSurfedge; ++i)
    {
        size_t SurfedgeIndex = static_cast<size_t>(Face.FirstSurfedgeIndex) + i;
        int32_t RawEdgeIndex = Lumps.m_LumpSurfedge.Surfedges[SurfedgeIndex];
        uint16_t VertexIndex1 = 0, VertexIndex2 = 0;
        if (RawEdgeIndex > 0)
        {
            uint32_t EdgeIndex = static_cast<uint32_t>(RawEdgeIndex);
            _ASSERTE(EdgeIndex < Lumps.m_LumpEdge.Edges.size());
            VertexIndex1 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[0];
            VertexIndex2 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[1];
        }
        else
        {
            uint32_t EdgeIndex = static_cast<uint32_t>(-static_cast<int64_t>(RawEdgeIndex));
            _ASSERTE(EdgeIndex < Lumps.m_LumpEdge.Edges.size());
            VertexIndex1 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[1];
            VertexIndex2 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[0];
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
        _ASSERTE(FaceVertexIndex < Lumps.m_LumpVertex.Vertices.size());
        SVec3 Vertex = Lumps.m_LumpVertex.Vertices[FaceVertexIndex];
        FaceVertices.emplace_back(Vertex.glmVec3());
    }

    return FaceVertices;
}

glm::vec3 CSceneReaderBsp::__getBspFaceNormal(size_t vFaceIndex)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < Lumps.m_LumpFace.Faces.size());
    const SBspFace& Face = Lumps.m_LumpFace.Faces[vFaceIndex];
    // get plane
    _ASSERTE(Face.PlaneIndex < Lumps.m_LumpPlane.Planes.size());
    const SBspPlane& Plane = Lumps.m_LumpPlane.Planes[Face.PlaneIndex];
    bool ReverseNormal = (Face.PlaneSide != 0);
    glm::vec3 Normal = Plane.Normal.glmVec3();
    if (ReverseNormal) Normal *= -1;
    return Normal;
}

std::vector<glm::vec2> CSceneReaderBsp::__getBspFaceUnnormalizedTexCoords(size_t vFaceIndex, std::vector<glm::vec3> vVertices)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < Lumps.m_LumpFace.Faces.size());
    const SBspFace& Face = Lumps.m_LumpFace.Faces[vFaceIndex];

    // find texture size
    uint16_t TexInfoIndex = Face.TexInfoIndex;
    _ASSERTE(TexInfoIndex < Lumps.m_LumpTexInfo.TexInfos.size());
    const SBspTexInfo& TexInfo = Lumps.m_LumpTexInfo.TexInfos[TexInfoIndex];

    std::vector<glm::vec2> TexCoords;
    for (const glm::vec3& Vertex : vVertices)
        TexCoords.emplace_back(TexInfo.getTexCoord(Vertex));
    return TexCoords;
}

// vTexCoords should contain unnormalized texcoords to avoid known of texture width and height
uint32_t CSceneReaderBsp::__getAndAppendBspFaceLightmap(size_t vFaceIndex, const std::vector<glm::vec2>& vTexCoords, std::vector<glm::vec2>& voLightmapCoords)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    voLightmapCoords.clear();

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < Lumps.m_LumpFace.Faces.size());
    const SBspFace& Face = Lumps.m_LumpFace.Faces[vFaceIndex];

    const float LightmapScale = 16.0f; // It should be 16.0 in GoldSrc. BTW, Source engine VHE seems to be able to change this.
    // TODO: handle lighting style like sky, no-draw, etc.
    if (Lumps.m_LumpLighting.Lightmaps.size() > 0 && Face.LightmapOffset < std::numeric_limits<uint32_t>::max())
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
            Lumps.m_LumpLighting.getRawRGBAPixels(Face.LightmapOffset / 3 + LightmapImageSize / 4 * i, LightmapImageSize / 4, pTempData);
            for (size_t k = 0; k < LightmapImageSize; ++k)
                pData[k] = std::max<uint8_t>(pData[k], pTempData[k]);
        }
        auto pLightmapImage = std::make_shared<CIOImage>();
        pLightmapImage->setImageSize(LightmapWidth, LightmapHeight);
        pLightmapImage->setData(pData);
        delete[] pTempData;
        delete[] pData;
        uint32_t LightmapIndex = m_Scene.LightmapImages.size();
        m_Scene.LightmapImages.emplace_back(pLightmapImage);

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

        return LightmapIndex;
    }
    else
    {
        return std::numeric_limits<uint32_t>::max();
    }
}

void CSceneReaderBsp::__getBspFaceTextureSizeAndName(size_t vFaceIndex, size_t& voWidth, size_t& voHeight, std::string& voName)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < Lumps.m_LumpFace.Faces.size());
    const SBspFace& Face = Lumps.m_LumpFace.Faces[vFaceIndex];

    uint16_t TexInfoIndex = Face.TexInfoIndex;
    _ASSERTE(TexInfoIndex < Lumps.m_LumpTexInfo.TexInfos.size());
    const SBspTexInfo& TexInfo = Lumps.m_LumpTexInfo.TexInfos[TexInfoIndex];
    _ASSERTE(TexInfo.TextureIndex < Lumps.m_LumpTexture.Textures.size());
    const SBspTexture& BspTexture = Lumps.m_LumpTexture.Textures[TexInfo.TextureIndex];

    voWidth = BspTexture.Width;
    voHeight = BspTexture.Height;
    voName = BspTexture.Name;
}

void CSceneReaderBsp::__appendBspFaceToObject(std::shared_ptr<S3DObject> pObject, uint32_t vFaceIndex)
{
    size_t TexWidth, TexHeight;
    std::string TexName;
    __getBspFaceTextureSizeAndName(vFaceIndex, TexWidth, TexHeight, TexName);
    std::vector<glm::vec3> Vertices = __getBspFaceVertices(vFaceIndex);
    glm::vec3 Normal = __getBspFaceNormal(vFaceIndex);
    std::vector<glm::vec2> TexCoords = __getBspFaceUnnormalizedTexCoords(vFaceIndex, Vertices);

    std::vector<glm::vec2> LightmapCoords;
    uint32_t LightmapIndex = __getAndAppendBspFaceLightmap(vFaceIndex, TexCoords, LightmapCoords);
    uint32_t TexIndex = m_TexNameToIndex[TexName];
    if (LightmapIndex == std::numeric_limits<uint32_t>::max())
        LightmapCoords.resize(TexCoords.size(), glm::vec2(0.0, 0.0));
    
    // scale texture coordinates
    for (glm::vec2& TexCoord : TexCoords)
    {
        TexCoord.x /= TexWidth;
        TexCoord.y /= TexHeight;
    }

    // save scaled datas
    for (size_t k = 2; k < Vertices.size(); ++k)
    {
        pObject->Vertices.emplace_back(Vertices[0] * m_SceneScale);
        pObject->Vertices.emplace_back(Vertices[k - 1] * m_SceneScale);
        pObject->Vertices.emplace_back(Vertices[k] * m_SceneScale);
        pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        pObject->Normals.emplace_back(Normal);
        pObject->Normals.emplace_back(Normal);
        pObject->Normals.emplace_back(Normal);
        pObject->TexCoords.emplace_back(TexCoords[0]);
        pObject->TexCoords.emplace_back(TexCoords[k - 1]);
        pObject->TexCoords.emplace_back(TexCoords[k]);
        pObject->LightmapCoords.emplace_back(LightmapCoords[0]);
        pObject->LightmapCoords.emplace_back(LightmapCoords[k - 1]);
        pObject->LightmapCoords.emplace_back(LightmapCoords[k]);
        pObject->TexIndices.emplace_back(TexIndex);
        pObject->TexIndices.emplace_back(TexIndex);
        pObject->TexIndices.emplace_back(TexIndex);
        pObject->LightmapIndices.emplace_back(LightmapIndex);
        pObject->LightmapIndices.emplace_back(LightmapIndex);
        pObject->LightmapIndices.emplace_back(LightmapIndex);
    }
}