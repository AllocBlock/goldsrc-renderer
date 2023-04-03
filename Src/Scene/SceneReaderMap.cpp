#include "SceneReaderMap.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "IOGoldSrcMap.h"

ptr<SSceneInfoGoldSrc> CSceneReaderMap::_readV()
{
    m_pSceneInfo = make<SSceneInfoGoldSrc>();
    m_pSceneInfo->pScene = make<CScene>();

    Scene::reportProgress(u8"[map]读取文件中");
    CIOGoldSrcMap Map = CIOGoldSrcMap(m_FilePath);
    if (!Map.read())
        throw std::runtime_error(u8"文件解析失败");
    std::vector<std::filesystem::path> WadPaths = Map.getWadPaths();
    std::vector<CIOGoldsrcWad> Wads = GoldSrc::readWads(WadPaths);

    Scene::reportProgress(u8"整理纹理中");
    // find used textures, load and index them
    std::map<std::string, uint32_t> TexNameToIndex;
    std::set<std::string> UsedTextureNames = Map.getUsedTextureNames();
    m_pSceneInfo->TexImageSet.push_back(Scene::generateBlackPurpleGrid(4, 4, 16));
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
                TexNameToIndex[TexName] = static_cast<uint32_t>(m_pSceneInfo->TexImageSet.size());

                ptr<CIOImage> pTexImage = Scene::getIOImageFromWad(Wad, Index.value());
                m_pSceneInfo->TexImageSet.emplace_back(std::move(pTexImage));
                break;
            }
        }
        if (!Found)
            TexNameToIndex[TexName] = 0;
    }

    Scene::reportProgress(u8"生成场景中");
    // group polygon by texture, one object per texture 
    std::vector<SMapPolygon> Polygons = Map.getAllPolygons();

    for (SMapPolygon& Polygon : Polygons)
    {
        uint32_t TexIndex = TexNameToIndex[Polygon.pPlane->TextureName];
        size_t TexWidth = m_pSceneInfo->TexImageSet[TexIndex]->getWidth();
        size_t TexHeight = m_pSceneInfo->TexImageSet[TexIndex]->getHeight();

        std::vector<glm::vec2> TexCoords = Polygon.getTexCoords(TexWidth, TexHeight);
        glm::vec3 Normal = Polygon.getNormal();
        
        // non-indexed data
        auto MeshData = CMeshData();
        auto pVertexArray = MeshData.getVertexArray();
        auto pColorArray = MeshData.getColorArray();
        auto pNormalArray = MeshData.getNormalArray();
        auto pTexCoordArray = MeshData.getTexCoordArray();
        auto pTexIndexArray = MeshData.getTexIndexArray();
        for (size_t k = 2; k < Polygon.Vertices.size(); ++k)
        {
            pVertexArray->append(Polygon.Vertices[0]);
            pVertexArray->append(Polygon.Vertices[k - 1]);
            pVertexArray->append(Polygon.Vertices[k]);
            pColorArray->append(glm::vec3(1.0, 1.0, 1.0), 3);
            pNormalArray->append(Normal, 3);
            pTexCoordArray->append(TexCoords[0]);
            pTexCoordArray->append(TexCoords[k - 1]);
            pTexCoordArray->append(TexCoords[k]);
            pTexIndexArray->append(TexIndex, 3);
        }

        // to y-up
        GoldSrc::toYupCounterClockwise(MeshData);

        auto pActor = GoldSrc::createActorByMeshAndTag(MeshData);
        m_pSceneInfo->pScene->addActor(pActor);
    }
    Scene::reportProgress(u8"完成");
    return m_pSceneInfo;
}
