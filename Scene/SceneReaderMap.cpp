#include "SceneReaderMap.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "IOGoldSrcMap.h"

std::shared_ptr<SScene> CSceneReaderMap::_readV()
{
    m_pScene = std::make_shared<SScene>();

    _reportProgress(u8"[map]读取文件中");
    CIOGoldSrcMap Map = CIOGoldSrcMap(m_FilePath);
    if (!Map.read())
        throw std::runtime_error(u8"文件解析失败");
    std::vector<std::filesystem::path> WadPaths = Map.getWadPaths();
    std::vector<CIOGoldsrcWad> Wads = readWads(WadPaths, m_ProgressReportFunc);

    _reportProgress(u8"整理纹理中");
    // find used textures, load and index them
    std::map<std::string, uint32_t> TexNameToIndex;
    std::set<std::string> UsedTextureNames = Map.getUsedTextureNames();
    m_pScene->TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
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
                TexNameToIndex[TexName] = m_pScene->TexImages.size();

                std::shared_ptr<CIOImage> pTexImage = getIOImageFromWad(Wad, Index.value());
                m_pScene->TexImages.emplace_back(std::move(pTexImage));
                break;
            }
        }
        if (!Found)
            TexNameToIndex[TexName] = 0;
    }

    _reportProgress(u8"生成场景中");
    // group polygon by texture, one object per texture 
    m_pScene->Objects.resize(UsedTextureNames.size());
    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        m_pScene->Objects[i] = std::make_shared<S3DObject>();
    }

    std::vector<SMapPolygon> Polygons = Map.getAllPolygons();

    for (SMapPolygon& Polygon : Polygons)
    {
        size_t TexIndex = TexNameToIndex[Polygon.pPlane->TextureName];
        size_t TexWidth = m_pScene->TexImages[TexIndex]->getImageWidth();
        size_t TexHeight = m_pScene->TexImages[TexIndex]->getImageHeight();
        std::shared_ptr<S3DObject> pObject = m_pScene->Objects[TexIndex];
        uint32_t IndexStart = pObject->Vertices.size();

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
        }
    }
    _reportProgress(u8"完成");
    return m_pScene;
}