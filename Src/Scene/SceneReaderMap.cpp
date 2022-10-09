#include "SceneReaderMap.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "IOGoldSrcMap.h"

using namespace Common;

ptr<SScene> CSceneReaderMap::_readV()
{
    m_pScene = make<SScene>();

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
    m_pScene->TexImageSet.push_back(Scene::generateBlackPurpleGrid(4, 4, 16));
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
                TexNameToIndex[TexName] = static_cast<uint32_t>(m_pScene->TexImageSet.size());

                ptr<CIOImage> pTexImage = Scene::getIOImageFromWad(Wad, Index.value());
                m_pScene->TexImageSet.emplace_back(std::move(pTexImage));
                break;
            }
        }
        if (!Found)
            TexNameToIndex[TexName] = 0;
    }

    Scene::reportProgress(u8"生成场景中");
    // group polygon by texture, one object per texture 
    m_pScene->Objects.resize(UsedTextureNames.size());
    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        m_pScene->Objects[i] = make<CMeshDataGoldSrc>();
    }

    std::vector<SMapPolygon> Polygons = Map.getAllPolygons();

    for (SMapPolygon& Polygon : Polygons)
    {
        uint32_t TexIndex = TexNameToIndex[Polygon.pPlane->TextureName];
        size_t TexWidth = m_pScene->TexImageSet[TexIndex]->getWidth();
        size_t TexHeight = m_pScene->TexImageSet[TexIndex]->getHeight();
        ptr<CMeshDataGoldSrc> pObject = m_pScene->Objects[TexIndex];
        uint32_t IndexStart = static_cast<uint32_t>(pObject->getVertexArray()->size());

        std::vector<glm::vec2> TexCoords = Polygon.getTexCoords(TexWidth, TexHeight);
        glm::vec3 Normal = Polygon.getNormal();

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
        auto pVertexArray = pObject->getVertexArray();
        auto pColorArray = pObject->getColorArray();
        auto pNormalArray = pObject->getNormalArray();
        auto pTexCoordArray = pObject->getTexCoordArray();
        auto pLightmapCoordArray = pObject->getLightmapCoordArray();
        auto pTexIndexArray = pObject->getTexIndexArray();
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
            pLightmapCoordArray->append(glm::vec2(0.0, 0.0), 3);
            pTexIndexArray->append(TexIndex, 3);
        }
    }
    Scene::reportProgress(u8"完成");
    return m_pScene;
}