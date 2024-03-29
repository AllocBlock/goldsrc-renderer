#include "SceneReaderBsp.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "IOGoldSrcWad.h"
#include "SingleValueDataArray.h"
#include "IOGoldSrcSpr.h"
#include "Environment.h"
#include "Log.h"
#include "ComponentMeshRenderer.h"
#include "ComponentIconRenderer.h"
#include "ComponentTextRenderer.h"

#include <sstream>

glm::vec3 stringToVec3(std::string vString)
{
    std::istringstream StringStream(vString);
    float X = 0.0f, Y = 0.0f, Z = 0.0f;
    StringStream >> X >> Y >> Z;
    return glm::vec3(X, Y, Z);
}

void CSceneReaderBsp::_readV(ptr<SSceneInfo> voSceneInfo)
{
    m_pTargetSceneInfo = voSceneInfo;
    
    __readBsp(m_FilePath);
    if (!m_Bsp.getLumps().m_LumpLighting.Lightmaps.empty())
    {
        m_pTargetSceneInfo->UseLightmap = true;
        m_pTargetSceneInfo->pLightmap = make<CLightmap>();
        m_HasLightmapData = true;
    }

    __readTextures();
    __loadBspTree();
    if (m_HasLightmapData)
        __correntLightmapCoords();

    __loadPointEntities();
    __loadSkyBox(m_FilePath.parent_path());
}

void CSceneReaderBsp::__readBsp(std::filesystem::path vFilePath)
{
    Scene::reportProgress(u8"[bsp]读取文件中");
    m_Bsp = CIOGoldSrcBsp(vFilePath);
    if (!m_Bsp.read())
        throw std::runtime_error(u8"文件解析失败");
}

void CSceneReaderBsp::__readTextures()
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    std::vector<ptr<CIOImage>> TexImageSet;

    m_TexNameToIndex.clear();
    // read wads
    const std::vector<std::filesystem::path>& WadPaths = Lumps.m_LumpEntity.WadPaths;
    std::vector<CIOGoldsrcWad> Wads = GoldSrc::readWads(WadPaths, m_FilePath.parent_path());

    // load textures
    // iterate each texture in texture lump
    // if bsp contains its data, load it, otherwise find it in all wads
    // if found, load it, otherwise set mapper to 0
    Scene::reportProgress(u8"整理纹理中");
    auto pDefaultImage = Scene::generateBlackPurpleGrid(4, 4, 16);
    pDefaultImage->setName("Default");
    TexImageSet.push_back(pDefaultImage);
    m_TexNameToIndex["TextureNotFound"] = 0;
    for (size_t i = 0; i < Lumps.m_LumpTexture.Textures.size(); ++i)
    {
        const SBspTexture& BspTexture = Lumps.m_LumpTexture.Textures[i];
        Scene::reportProgress(u8"读取纹理（" + std::to_string(i + 1) + "/" + std::to_string(Lumps.m_LumpTexture.Textures.size()) + " " + BspTexture.Name + u8"）");
        if (BspTexture.IsDataInBsp)
        {
            ptr<CIOImage> pTexImage = Scene::getIOImageFromBspTexture(BspTexture);
            m_TexNameToIndex[BspTexture.Name] = static_cast<uint32_t>(TexImageSet.size());
            TexImageSet.emplace_back(std::move(pTexImage));
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
                    ptr<CIOImage> pTexImage = Scene::getIOImageFromWad(Wad, Index.value());
                    m_TexNameToIndex[BspTexture.Name] = static_cast<uint32_t>(TexImageSet.size());
                    TexImageSet.emplace_back(std::move(pTexImage));
                    break;
                }
            }
            if (!Found)
                m_TexNameToIndex[BspTexture.Name] = 0;
        }
    }

    m_pTargetSceneInfo->TexImageSet = std::move(TexImageSet);
}

void CSceneReaderBsp::__loadLeaf(size_t vLeafIndex, CMeshData& vioMeshDataNormal, CMeshData& vioMeshDataSky)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    _ASSERTE(vLeafIndex < Lumps.m_LumpLeaf.Leaves.size());
    const SBspLeaf& Leaf = Lumps.m_LumpLeaf.Leaves[vLeafIndex];

    size_t TexWidth, TexHeight;
    std::string TexName;
    
    for (uint16_t i = 0; i < Leaf.NumMarkSurface; ++i)
    {
        uint16_t MarkSurfaceIndex = Leaf.FirstMarkSurfaceIndex + i;
        _ASSERTE(MarkSurfaceIndex < Lumps.m_LumpMarkSurface.FaceIndices.size());
        uint16_t FaceIndex = Lumps.m_LumpMarkSurface.FaceIndices[MarkSurfaceIndex];
        __getBspFaceTextureSizeAndName(FaceIndex, TexWidth, TexHeight, TexName);
        if (TexName == "sky")
            __appendBspFaceToObject(vioMeshDataSky, FaceIndex, true);
        else
            __appendBspFaceToObject(vioMeshDataNormal, FaceIndex, true);
    }
}

CActor::Ptr CSceneReaderBsp::__loadEntity(size_t vModelIndex)
{
    const SBspLumps& Lumps = m_Bsp.getLumps();

    auto MeshData = CMeshData();
    
    const SBspModel& Model = Lumps.m_LumpModel.Models[vModelIndex];
    size_t TexWidth, TexHeight;
    std::string TexName;
    for (uint16_t i = 0; i < Model.NumFaces; ++i)
    {
        uint16_t FaceIndex = Model.FirstFaceIndex + i;
        __getBspFaceTextureSizeAndName(FaceIndex, TexWidth, TexHeight, TexName);
        _ASSERTE(TexName != "sky"); // model should not contain sky face
        __appendBspFaceToObject(MeshData, FaceIndex, true);
    }

    // to y-up counter clock wise
    GoldSrc::toYupCounterClockwise(MeshData);

    auto pActor = GoldSrc::createActorByMeshAndTag(MeshData, { "entity" });
    return pActor;
}

void CSceneReaderBsp::__loadBspTree()
{
    auto pScene = m_pTargetSceneInfo->pScene;

    const SBspLumps& Lumps = m_Bsp.getLumps();

    // read node and PVS data
    Scene::reportProgress(u8"载入BSP数据");
    size_t NodeNum = Lumps.m_LumpNode.Nodes.size();
    size_t LeafNum = Lumps.m_LumpLeaf.Leaves.size();
    size_t ModelNum = Lumps.m_LumpModel.Models.size();

    // read nodes and leaves
    auto MeshDataNormalPart = CMeshData();
    auto MeshDataSkyPart = CMeshData();
    for (size_t i = 0; i < NodeNum; ++i)
    {
        const SBspNode& OriginNode = Lumps.m_LumpNode.Nodes[i];
        _ASSERTE(OriginNode.PlaneIndex < Lumps.m_LumpPlane.Planes.size());
        const SBspPlane& OriginPlane = Lumps.m_LumpPlane.Planes[OriginNode.PlaneIndex];
        
        if (OriginNode.ChildrenIndices[0] <= 0)
        {
            size_t LeafIndex = static_cast<size_t>(~OriginNode.ChildrenIndices[0]);
            __loadLeaf(LeafIndex, MeshDataNormalPart, MeshDataSkyPart);
        }
        if (OriginNode.ChildrenIndices[1] <= 0)
        {
            size_t LeafIndex = static_cast<size_t>(~OriginNode.ChildrenIndices[1]);
            __loadLeaf(LeafIndex, MeshDataNormalPart, MeshDataSkyPart);
        }
    }

    _ASSERTE(MeshDataNormalPart.getLightmapIndexArray()->empty() || MeshDataNormalPart.getVertexArray()->size() == MeshDataNormalPart.getLightmapIndexArray()->size()); // all do not has lightmap or all have lightmap

    // to y-up counter clock wise
    GoldSrc::toYupCounterClockwise(MeshDataNormalPart);
    auto pActorNormalPart = GoldSrc::createActorByMeshAndTag(MeshDataNormalPart, { "brush" });
    pActorNormalPart->setName("brush");
    m_pTargetSceneInfo->pScene->addActor(pActorNormalPart);

    // if need sky part, uncomment this
    //GoldSrc::toYupCounterClockwise(MeshDataSkyPart);
    //auto pActorSkyPart = GoldSrc::createActorByMeshAndTag(MeshDataSkyPart, { "sky" });
    //pActorSkyPart->setName("sky");
    //pActorSkyPart->setVisible(false);
    //m_pSceneInfo->pScene->addActor(pActorSkyPart);
    
    // read models
    for (size_t i = 1; i < ModelNum; ++i) // 从1号开始，0号实体似乎就是worldspawn，不含实体属性，包含全部固体
    {
        // load entities data and calculate bounding box
        auto pModelActor = __loadEntity(i);
        pScene->addActor(pModelActor);
        pModelActor->setName("model " + std::to_string(i));
        
        // get model entity name
        std::optional<SMapEntity> EntityOpt = __findEntity(i);
        if (EntityOpt.has_value())
        {
            const SMapEntity& Entity = EntityOpt.value();
            if (Entity.Properties.find("classname") != Entity.Properties.end())
            {
                pModelActor->setName(Entity.Properties.at("classname"));
            }
        }
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
        const GoldSrc::SVec3& Vertex = Lumps.m_LumpVertex.Vertices[FaceVertexIndex];
        FaceVertices.emplace_back(Vertex.toGlm());
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
    glm::vec3 Normal = Plane.Normal.toGlm();
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
// return CMeshData::InvalidLightmapIndex, {} if no lightmap found
std::pair<uint32_t, std::vector<glm::vec2>> CSceneReaderBsp::__getAndAppendBspFaceLightmap(size_t vFaceIndex, const std::vector<glm::vec2>& vTexCoords)
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
        auto pLightmapImage = make<CIOImage>();
        pLightmapImage->setSize(LightmapWidth, LightmapHeight);
        pLightmapImage->setChannelNum(4);
        pLightmapImage->setData(pIndices);
        delete[] pTempData;
        delete[] pIndices;
        uint32_t LightmapIndex = static_cast<uint32_t>(m_pTargetSceneInfo->pLightmap->appendLightmap(pLightmapImage));

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
        return std::make_pair(CMeshData::InvalidLightmapIndex, std::vector<glm::vec2>{});
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

void CSceneReaderBsp::__appendBspFaceToObject(CMeshData& vioMeshData, uint32_t vFaceIndex, bool vForceFillLightmapData)
{
    size_t TexWidth, TexHeight;
    std::string TexName;
    __getBspFaceTextureSizeAndName(vFaceIndex, TexWidth, TexHeight, TexName);

    std::vector<glm::vec3> Vertices = __getBspFaceVertices(vFaceIndex);
    glm::vec3 Normal = __getBspFaceNormal(vFaceIndex);
    std::vector<glm::vec2> TexCoords = __getBspFaceUnnormalizedTexCoords(vFaceIndex, Vertices);

    auto [LightmapIndex, LightmapCoords] = __getAndAppendBspFaceLightmap(vFaceIndex, TexCoords);
    uint32_t TexIndex = m_TexNameToIndex[TexName];

    bool HasLightMap = (LightmapIndex != CMeshData::InvalidLightmapIndex);
    
    // scale texture coordinates
    for (glm::vec2& TexCoord : TexCoords)
    {
        TexCoord.x /= TexWidth;
        TexCoord.y /= TexHeight;
    }

    // save scaled datas
    auto pVertexArray = vioMeshData.getVertexArray();
    auto pColorArray = vioMeshData.getColorArray();
    auto pNormalArray = vioMeshData.getNormalArray();
    auto pTexCoordArray = vioMeshData.getTexCoordArray();
    auto pTexIndexArray = vioMeshData.getTexIndexArray();
    auto pLightmapIndexArray = vioMeshData.getLightmapIndexArray();
    auto pLightmapCoordArray = vioMeshData.getLightmapTexCoordArray();
    for (size_t k = 2; k < Vertices.size(); ++k)
    {
        size_t VertexStartIndex = (k - 2) * 3;
        pVertexArray->append(Vertices[0] * m_SceneScale);
        pVertexArray->append(Vertices[k - 1] * m_SceneScale);
        pVertexArray->append(Vertices[k] * m_SceneScale);
        pColorArray->append(glm::vec3(1.0, 1.0, 1.0), 3);
        pNormalArray->append(Normal, 3);
        pTexCoordArray->append(TexCoords[0]);
        pTexCoordArray->append(TexCoords[k - 1]);
        pTexCoordArray->append(TexCoords[k]);
        pTexIndexArray->append(TexIndex, 3);

        if (HasLightMap)
        {
            pLightmapIndexArray->append(LightmapIndex, 3);
            pLightmapCoordArray->append(LightmapCoords[0]);
            pLightmapCoordArray->append(LightmapCoords[k - 1]);
            pLightmapCoordArray->append(LightmapCoords[k]);
        }
        else if (vForceFillLightmapData)
        {
            pLightmapIndexArray->append(0, 3);
            pLightmapCoordArray->append(glm::vec2(0.0f), 3);
        }
    }
}

void CSceneReaderBsp::__correntLightmapCoords()
{
    Scene::reportProgress(u8"修正Lightmap数据");

    for (size_t i = 0; i < m_pTargetSceneInfo->pScene->getActorNum(); ++i)
    {
        auto pActor = m_pTargetSceneInfo->pScene->getActor(i);

        auto pTransform = pActor->getTransform();
        auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
        if (!pMeshRenderer) continue;

        auto pMesh = pMeshRenderer->getMesh();
        if (!pMesh) continue;

        const auto& MeshData = pMesh->getMeshDataV();

        if (!MeshData.hasLightmap()) continue;

        auto pLightmapIndexArray = MeshData.getLightmapIndexArray();
        auto pLightmapTexCoordArray = MeshData.getLightmapTexCoordArray();

        for (size_t i = 0; i < pLightmapIndexArray->size(); ++i)
        {
            uint32_t LightmapIndex = pLightmapIndexArray->get(i);
            _ASSERTE(LightmapIndex != CMeshData::InvalidLightmapIndex);

            pLightmapTexCoordArray->set(i, m_pTargetSceneInfo->pLightmap->getAcutalLightmapCoord(LightmapIndex, pLightmapTexCoordArray->get(i)));
        }
    }
}

void CSceneReaderBsp::__loadSkyBox(std::filesystem::path vCurrentDir)
{
    Scene::reportProgress(u8"载入天空盒");

    const SBspLumps& Lumps = m_Bsp.getLumps();

    std::string SkyFilePrefix = Lumps.m_LumpEntity.SkyBoxPrefix;
    if (SkyFilePrefix.empty())
    {
        Log::log("地图未指定天空文件，已使用默认天空neb6");
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
        Log::log("未找到天空图片文件[" + SkyFilePrefix + "]，将不会渲染天空盒");
}

bool CSceneReaderBsp::__readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension, std::filesystem::path vCurrentDir)
{
    // front back up down right left
    std::array<std::string, 6> SkyBoxPostfixes = { "ft", "bk", "up", "dn", "rt", "lf" };
    for (size_t i = 0; i < SkyBoxPostfixes.size(); ++i)
    {
        std::filesystem::path ImagePath;
        if (Environment::findFile(vSkyFilePrefix + SkyBoxPostfixes[i] + vExtension, vCurrentDir, true, ImagePath))
        {
            m_pTargetSceneInfo->SkyBoxImages[i] = make<CIOImage>();
            m_pTargetSceneInfo->SkyBoxImages[i]->read(ImagePath);
        }
        else
        {
            m_pTargetSceneInfo->UseSkyBox = false;
            m_pTargetSceneInfo->SkyBoxImages = {};
            return false;
        }
    }
    m_pTargetSceneInfo->UseSkyBox = true;
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

void CSceneReaderBsp::__loadPointEntities()
{
    auto EntityLump = m_Bsp.getLumps().m_LumpEntity;
    for (const auto& Entity : EntityLump.Entities)
    {
        if (!Entity.Brushes.empty()) continue;

        std::string Name;
        if (Entity.Properties.find("classname") != Entity.Properties.end())
        {
            Name = Entity.Properties.at("classname");
        }

        glm::vec3 Origin = glm::vec3(0.0, 0.0, 0.0);
        if (Entity.Properties.find("origin") != Entity.Properties.end())
        {
            Origin = stringToVec3(Entity.Properties.at("origin"));
        }

        if (Name == "env_sprite")
        {
            SGoldSrcSprite Sprite;
            Sprite.Position = Origin * m_SceneScale;
            Sprite.Angle = (Entity.Properties.find("angles") != Entity.Properties.end()) ? stringToVec3(Entity.Properties.at("angles")) : glm::vec3(0.0f, 0.0f, 0.0f);
            Sprite.Scale = (Entity.Properties.find("scale") != Entity.Properties.end()) ? std::atoi(Entity.Properties.at("scale").c_str()) : 1.0f;
            std::string Name = (Entity.Properties.find("model") != Entity.Properties.end()) ? Entity.Properties.at("model") : "";

            auto RequestResult = Scene::requestFilePath(Name, m_FilePath.parent_path(), u8"需要图标文件：" + Name, "spr");
            if (RequestResult.Found)
            {
                CIOGoldSrcSpr Spr;
                Spr.read(RequestResult.Data);
                uint32_t Width = 0, Height = 0;
                Spr.getFrameSize(0, Width, Height);
                auto pImage = make<CIOImage>();
                void* pData = new uint8_t[Width * Height * 4];
                Spr.getFrameRGBAPixels(0, pData);
                pImage->setSize(Width, Height);
                pImage->setData(pData);

                Sprite.pImage = pImage;
                Sprite.Type = static_cast<EGoldSrcSpriteType>(Spr.getType());
                m_pTargetSceneInfo->SprSet.emplace_back(Sprite);
            }
        }
        
        auto pPointEntityActor = make<CActor>();
        pPointEntityActor->getTransform()->setTranslate(GoldSrc::toYup(Origin * m_SceneScale));
        pPointEntityActor->getTransform()->setScale(glm::vec3(0.2f));
        pPointEntityActor->addTag("point_entity");
        if (!Name.empty())
        {
            pPointEntityActor->setName(Entity.Properties.at("classname"));
        }

        auto pIconRenderer = make<CComponentIconRenderer>();
        pIconRenderer->setIcon(EIcon::TIP);
        pPointEntityActor->getTransform()->addComponent(pIconRenderer);

        auto pTextRenderer = make<CComponentTextRenderer>();
        pTextRenderer->setOffset(glm::vec2(0, 1.2f));
        pTextRenderer->setText(Name);
        pTextRenderer->setScale(0.5f);
        pPointEntityActor->getTransform()->addComponent(pTextRenderer);

        m_pTargetSceneInfo->pScene->addActor(pPointEntityActor);
    }
}
