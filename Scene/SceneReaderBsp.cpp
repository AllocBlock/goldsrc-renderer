#include "SceneReaderBsp.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "IOGoldSrcWad.h"

std::shared_ptr<SScene> CSceneReaderBsp::read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    m_ProgressReportFunc = vProgressReportFunc;
    m_pScene = std::make_shared<SScene>();
    
    __readBsp(vFilePath);
    if (!m_Bsp.getLumps().m_LumpLighting.Lightmaps.empty())
    {
        m_pScene->UseLightmap = true;
        m_pScene->pLightmap = std::make_shared<CLightmap>();
        m_HasLightmapData = true;
    }

    __readTextures();
    __loadBspTree();
    __loadBspPvs();
    if (m_HasLightmapData)
        __correntLightmapCoords();

    __loadSkyBox(vFilePath.parent_path());

    return m_pScene;
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
    const SBspLumps& Lumps = m_Bsp.getLumps();

    std::vector<std::shared_ptr<CIOImage>> TexImages;

    m_TexNameToIndex.clear();
    // read wads
    const std::vector<std::filesystem::path>& WadPaths = Lumps.m_LumpEntity.WadPaths;
    std::vector<CIOGoldsrcWad> Wads = readWads(WadPaths, m_ProgressReportFunc);

    // load textures
    // iterate each texture in texture lump
    // if bsp contains its data, load it, otherwise find it in all wads
    // if found, load it, otherwise set mapper to 0
    __reportProgress(u8"整理纹理中");
    TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    m_TexNameToIndex["TextureNotFound"] = 0;
    for (size_t i = 0; i < Lumps.m_LumpTexture.Textures.size(); ++i)
    {
        const SBspTexture& BspTexture = Lumps.m_LumpTexture.Textures[i];
        __reportProgress(u8"读取纹理（" + std::to_string(i + 1) + "/" + std::to_string(Lumps.m_LumpTexture.Textures.size()) + " " + BspTexture.Name + u8"）");
        if (BspTexture.IsDataInBsp)
        {
            std::shared_ptr<CIOImage> pTexImage = getIOImageFromBspTexture(BspTexture);
            m_TexNameToIndex[BspTexture.Name] = TexImages.size();
            TexImages.emplace_back(std::move(pTexImage));
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
                    m_TexNameToIndex[BspTexture.Name] = TexImages.size();
                    TexImages.emplace_back(std::move(pTexImage));
                    break;
                }
            }
            if (!Found)
                m_TexNameToIndex[BspTexture.Name] = 0;
        }
    }

    m_pScene->TexImages = std::move(TexImages);
}

std::vector<std::shared_ptr<S3DObject>> CSceneReaderBsp::__loadLeaf(size_t vLeafIndex)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vLeafIndex < Lumps.m_LumpLeaf.Leaves.size());
    const SBspLeaf& Leaf = Lumps.m_LumpLeaf.Leaves[vLeafIndex];

    auto pObjectNormalPart = std::make_shared<S3DObject>();
    pObjectNormalPart->RenderType = E3DObjectRenderType::NORMAL;
    auto pObjectSkyPart = std::make_shared<S3DObject>();
    pObjectSkyPart->RenderType = E3DObjectRenderType::SKY;

    size_t TexWidth, TexHeight;
    std::string TexName;
    
    for (uint16_t i = 0; i < Leaf.NumMarkSurface; ++i)
    {
        uint16_t MarkSurfaceIndex = Leaf.FirstMarkSurfaceIndex + i;
        _ASSERTE(MarkSurfaceIndex < Lumps.m_LumpMarkSurface.FaceIndices.size());
        uint16_t FaceIndex = Lumps.m_LumpMarkSurface.FaceIndices[MarkSurfaceIndex];
        __getBspFaceTextureSizeAndName(FaceIndex, TexWidth, TexHeight, TexName);
        if (TexName == "sky")
        {
            __appendBspFaceToObject(pObjectSkyPart, FaceIndex);
        }
        else
        {
            __appendBspFaceToObject(pObjectNormalPart, FaceIndex);
        }
    }

    std::vector<std::shared_ptr<S3DObject>> Objects;
    if (!pObjectNormalPart->Vertices.empty())
        Objects.emplace_back(std::move(pObjectNormalPart));
    if (!pObjectSkyPart->Vertices.empty())
        Objects.emplace_back(std::move(pObjectSkyPart));
    return Objects;
}

std::vector<std::shared_ptr<S3DObject>> CSceneReaderBsp::__loadEntity(size_t vModelIndex)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    auto pObjectNormalPart = std::make_shared<S3DObject>();
    pObjectNormalPart->RenderType = E3DObjectRenderType::NORMAL;
    auto pObjectSkyPart = std::make_shared<S3DObject>();
    pObjectSkyPart->RenderType = E3DObjectRenderType::SKY;
    
    const SBspModel& Model = Lumps.m_LumpModel.Models[vModelIndex];
    size_t TexWidth, TexHeight;
    std::string TexName;
    for (uint16_t i = 0; i < Model.NumFaces; ++i)
    {
        uint16_t FaceIndex = Model.FirstFaceIndex + i;
        __getBspFaceTextureSizeAndName(FaceIndex, TexWidth, TexHeight, TexName);
        if (TexName == "sky")
            __appendBspFaceToObject(pObjectSkyPart, FaceIndex);
        else
            __appendBspFaceToObject(pObjectNormalPart, FaceIndex);
    }

    std::vector<std::shared_ptr<S3DObject>> Objects;
    Objects.emplace_back(std::move(pObjectNormalPart));
    Objects.emplace_back(std::move(pObjectSkyPart));
    return std::move(Objects);
}

void CSceneReaderBsp::__loadBspTree()
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    SBspTree BspTree;
    std::vector<std::shared_ptr<S3DObject>> Objects;

    // read node and PVS data
    __reportProgress(u8"载入BSP数据");
    size_t NodeNum = Lumps.m_LumpNode.Nodes.size();
    size_t LeafNum = Lumps.m_LumpLeaf.Leaves.size();
    size_t ModelNum = Lumps.m_LumpModel.Models.size();

    BspTree.NodeNum = NodeNum;
    BspTree.LeafNum = LeafNum;
    BspTree.ModelNum = ModelNum;
    BspTree.Nodes.resize(NodeNum + LeafNum);

    // read nodes and leaves
    for (size_t i = 0; i < NodeNum; ++i)
    {
        const SBspNode& OriginNode = Lumps.m_LumpNode.Nodes[i];
        _ASSERTE(OriginNode.PlaneIndex < Lumps.m_LumpPlane.Planes.size());
        const SBspPlane& OriginPlane = Lumps.m_LumpPlane.Planes[OriginNode.PlaneIndex];
        SBspTreeNode& Node = BspTree.Nodes[i];

        Node.PlaneNormal = OriginPlane.Normal.glmVec3();
        Node.PlaneDistance = OriginPlane.DistanceToOrigin * m_SceneScale;

        if (OriginNode.ChildrenIndices[0] > 0)
            Node.Front = OriginNode.ChildrenIndices[0];
        else
        {
            size_t LeafIndex = static_cast<size_t>(~OriginNode.ChildrenIndices[0]);
            Node.Front = NodeNum + LeafIndex;
            std::vector<std::shared_ptr<S3DObject>> LeafObjects = __loadLeaf(LeafIndex);
            std::vector<size_t> ObjectIndices;
            for (std::shared_ptr<S3DObject> LeafObject : LeafObjects)
            {
                ObjectIndices.emplace_back(Objects.size());
                Objects.emplace_back(LeafObject);
            }
            BspTree.LeafIndexToObjectIndices[LeafIndex] = ObjectIndices;
        }
        if (OriginNode.ChildrenIndices[1] > 0)
            Node.Back = OriginNode.ChildrenIndices[1];
        else
        {
            size_t LeafIndex = static_cast<size_t>(~OriginNode.ChildrenIndices[1]);
            Node.Back = NodeNum + LeafIndex;
            std::vector<std::shared_ptr<S3DObject>> LeafObjects = __loadLeaf(LeafIndex);
            std::vector<size_t> ObjectIndices;
            for (std::shared_ptr<S3DObject> pLeafObject : LeafObjects)
            {
                ObjectIndices.emplace_back(Objects.size());
                Objects.emplace_back(pLeafObject);
            }
            BspTree.LeafIndexToObjectIndices[LeafIndex] = ObjectIndices;
        }
    }

    // read models
    BspTree.ModelInfos.resize(ModelNum);
    for (size_t i = 0; i < ModelNum; ++i)
    {
        // get entity opacity
        std::optional<SMapEntity> EntityOpt = __findEntity(i);
        bool IsTransparent = false;
        float Opacity = 1.0f;
        if (EntityOpt.has_value())
        {
            const SMapEntity& Entity = EntityOpt.value();
            int RenderMode = Entity.Properties.find("rendermode") != Entity.Properties.end() ? std::stoi(Entity.Properties.at("rendermode")) : 0;
            Opacity = Entity.Properties.find("renderamt") != Entity.Properties.end() ? std::stof(Entity.Properties.at("renderamt")) / 255.0f : 1.0f;

            IsTransparent = (RenderMode != 0); // half transparent or indexed transparent
        }

        // load entities data and calculate bounding box
        std::vector<std::shared_ptr<S3DObject>> ModelObjects = __loadEntity(i);
        std::vector<size_t> ObjectIndices;
        S3DBoundingBox TotalBoundingBox =
        {
            {INFINITY, INFINITY, INFINITY},
            {-INFINITY, -INFINITY, -INFINITY},
        };
        for (std::shared_ptr<S3DObject> pModelObject : ModelObjects)
        {
            std::optional<S3DBoundingBox> BoundingBox = pModelObject->getBoundingBox();
            if (BoundingBox == std::nullopt) continue;
            TotalBoundingBox.Min.x = std::min<float>(TotalBoundingBox.Min.x, BoundingBox.value().Min.x);
            TotalBoundingBox.Min.y = std::min<float>(TotalBoundingBox.Min.y, BoundingBox.value().Min.y);
            TotalBoundingBox.Min.z = std::min<float>(TotalBoundingBox.Min.z, BoundingBox.value().Min.z);
            TotalBoundingBox.Max.x = std::max<float>(TotalBoundingBox.Max.x, BoundingBox.value().Max.x);
            TotalBoundingBox.Max.y = std::max<float>(TotalBoundingBox.Max.y, BoundingBox.value().Max.y);
            TotalBoundingBox.Max.z = std::max<float>(TotalBoundingBox.Max.z, BoundingBox.value().Max.z);
            ObjectIndices.emplace_back(Objects.size());
            Objects.emplace_back(pModelObject);
        }

        BspTree.ModelIndexToObjectIndex[i] = ObjectIndices;
        BspTree.ModelInfos[i].BoundingBox = TotalBoundingBox;
        BspTree.ModelInfos[i].IsTransparent = IsTransparent;
        BspTree.ModelInfos[i].Opacity = Opacity;
    }

    // load pvs data to leaves
    for (size_t i = 0; i < LeafNum; ++i)
    {
        const SBspLeaf& OriginLeaf = Lumps.m_LumpLeaf.Leaves[i];
        SBspTreeNode& Node = BspTree.Nodes[NodeNum + i];

        if (OriginLeaf.VisOffset >= 0)
            Node.PvsOffset = OriginLeaf.VisOffset;
    }

    m_pScene->BspTree = BspTree;
    m_pScene->Objects = Objects;
}

void CSceneReaderBsp::__loadBspPvs()
{
    __reportProgress(u8"载入VIS数据");

    const SBspLumps& Lumps = m_Bsp.getLumps();

    if (!Lumps.m_LumpVisibility.Vis.empty())
    {
        m_pScene->UsePVS = true;
        m_pScene->BspPvs.decompress(Lumps.m_LumpVisibility.Vis, m_pScene->BspTree);
    }
}

std::vector<glm::vec3> CSceneReaderBsp::__getBspFaceVertices(size_t vFaceIndex)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < Lumps.m_LumpFace.Faces.size());
    const SBspFace& Face = Lumps.m_LumpFace.Faces[vFaceIndex];
    _ASSERTE(static_cast<size_t>(Face.FirstSurfedgeIndex) + Face.NumSurfedge <= Lumps.m_LumpSurfedge.EdgeIndices.size());

    // extract face vertex index from edges
    std::vector<uint16_t> FaceVertexIndices;
    uint16_t LastVertexIndex2 = 0;
    for (uint16_t i = 0; i < Face.NumSurfedge; ++i)
    {
        size_t SurfedgeIndex = static_cast<size_t>(Face.FirstSurfedgeIndex) + i;
        int32_t RawEdgeIndex = Lumps.m_LumpSurfedge.EdgeIndices[SurfedgeIndex];
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
        const IOCommon::SGoldSrcVec3& Vertex = Lumps.m_LumpVertex.Vertices[FaceVertexIndex];
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
std::pair<std::optional<size_t>, std::vector<glm::vec2>> CSceneReaderBsp::__getAndAppendBspFaceLightmap(size_t vFaceIndex, const std::vector<glm::vec2>& vTexCoords)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vFaceIndex >= 0 && vFaceIndex < Lumps.m_LumpFace.Faces.size());
    const SBspFace& Face = Lumps.m_LumpFace.Faces[vFaceIndex];

    const float LightmapScale = 16.0f; // It should be 16.0 in GoldSrc. BTW, Source engine VHE seems to be able to change this.

    if (m_HasLightmapData && Face.LightmapOffset < std::numeric_limits<uint32_t>::max())
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
        uint8_t* pIndices = new uint8_t[LightmapImageSize];
        std::memset(pIndices, 0, LightmapImageSize);
        uint8_t* pTempData = new uint8_t[LightmapImageSize];
        // blend all lightmap, use the brightest value
        for (int i = 0; i < 4; ++i)
        {
            if (Face.LightingStyles[i] == 0xff) continue;
            Lumps.m_LumpLighting.getRawRGBAPixels(Face.LightmapOffset / 3 + LightmapImageSize / 4 * i, LightmapImageSize / 4, pTempData);
            for (size_t k = 0; k < LightmapImageSize; ++k)
                pIndices[k] = std::max<uint8_t>(pIndices[k], pTempData[k]);
        }
        auto pLightmapImage = std::make_shared<CIOImage>();
        pLightmapImage->setImageSize(LightmapWidth, LightmapHeight);
        pLightmapImage->setImageChannels(4);
        pLightmapImage->setData(pIndices);
        delete[] pTempData;
        delete[] pIndices;
        uint32_t LightmapIndex = static_cast<uint32_t>(m_pScene->pLightmap->appendLightmap(pLightmapImage));

        std::vector<glm::vec2> LightmapCoords;
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

            LightmapCoords.emplace_back(LightmapCoord);
        }

        return std::make_pair(LightmapIndex, LightmapCoords);
    }
    else
    {
        return std::make_pair(std::nullopt, std::vector<glm::vec2>{});
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

    auto [LightmapIndex, LightmapCoords] = __getAndAppendBspFaceLightmap(vFaceIndex, TexCoords);
    uint32_t TexIndex = m_TexNameToIndex[TexName];
    if (LightmapIndex == std::nullopt)
    {
        LightmapCoords.resize(TexCoords.size(), glm::vec2(0.0, 0.0));
    }
    else
    {
        pObject->HasLightmap = true;
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

void CSceneReaderBsp::__correntLightmapCoords()
{
    __reportProgress(u8"修正Lightmap数据");

    for (auto& pObject : m_pScene->Objects)
    {
        if (!pObject->HasLightmap) continue;

        for (size_t i = 0; i < pObject->LightmapCoords.size(); ++i)
        {
            if (pObject->LightmapIndices[i] == std::nullopt) continue;

            size_t LightmapIndex = pObject->LightmapIndices[i].value();
            pObject->LightmapCoords[i] = m_pScene->pLightmap->getAcutalLightmapCoord(LightmapIndex, pObject->LightmapCoords[i]);
        }
    }
}

void CSceneReaderBsp::__loadSkyBox(std::filesystem::path vCurrentDir)
{
    __reportProgress(u8"载入天空盒");

    const SBspLumps& Lumps = m_Bsp.getLumps();

    std::string SkyFilePrefix = Lumps.m_LumpEntity.SkyBoxPrefix;
    if (SkyFilePrefix.empty())
    {
        Common::Log::log(u8"地图未指定天空文件，已使用默认天空neb6");
        vCurrentDir = std::filesystem::current_path().string() + "/../data/";
        SkyFilePrefix = "neb6";
    }
    
    std::vector<std::string> Extensions = { ".tga", ".bmp", ".png", ".jpg" };

    bool FoundSkyBoxImages = false;
    for (const std::string& Extension : Extensions)
    {
        if (__readSkyboxImages(SkyFilePrefix, Extension, vCurrentDir))
        {
            FoundSkyBoxImages = true;
            break;
        }
    }
    if (!FoundSkyBoxImages)
        Common::Log::log(u8"未找到天空图片文件[" + SkyFilePrefix + u8"]，将不会渲染天空盒");
}

bool CSceneReaderBsp::__readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension, std::filesystem::path vCurrentDir)
{
    // front back up down right left
    std::array<std::string, 6> SkyBoxPostfixes = { "ft", "bk", "up", "dn", "rt", "lf" };
    for (size_t i = 0; i < SkyBoxPostfixes.size(); ++i)
    {
        std::filesystem::path ImagePath;
        if (findFile(vSkyFilePrefix + SkyBoxPostfixes[i] + vExtension, vCurrentDir, ImagePath))
        {
            m_pScene->SkyBoxImages[i] = std::make_shared<CIOImage>();
            m_pScene->SkyBoxImages[i]->read(ImagePath);
        }
        else
        {
            m_pScene->UseSkyBox = false;
            m_pScene->SkyBoxImages = {};
            return false;
        }
    }
    m_pScene->UseSkyBox = true;
    return true;
}

std::optional<SMapEntity> CSceneReaderBsp::__findEntity(size_t vModelIndex)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    const std::vector<SMapEntity>& Entities = Lumps.m_LumpEntity.Entities;
    _ASSERTE(vModelIndex < Lumps.m_LumpModel.Models.size());
    const SBspModel& Model = Lumps.m_LumpModel.Models[vModelIndex];

    for (const SMapEntity& Entity : Entities)
    {
        if (Entity.Properties.find("model") != Entity.Properties.end()
            && Entity.Properties.at("model") == "*" + std::to_string(vModelIndex))
        {
            return Entity;
        }
    }
    return std::nullopt;
}