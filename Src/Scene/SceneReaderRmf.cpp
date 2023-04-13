#include "SceneReaderRmf.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

void CSceneReaderRmf::_readV(ptr<SSceneInfo> voSceneInfo)
{
    m_pTargetSceneInfo = voSceneInfo;

    __readRmf(m_FilePath);
    __readWadsAndInitTextures();
    Scene::reportProgress(u8"读取场景中");
    __readObject(m_Rmf.getWorld());
}

void CSceneReaderRmf::__readRmf(std::filesystem::path vFilePath)
{
    Scene::reportProgress(u8"[rmf]读取文件中");
    m_Rmf = CIOGoldSrcRmf(vFilePath);
    if (!m_Rmf.read())
        throw std::runtime_error(u8"文件解析失败");
}

void CSceneReaderRmf::__readWadsAndInitTextures()
{
    // read wads
    Scene::reportProgress(u8"初始化纹理中");
    m_TexNameToIndex.clear();
    
    Scene::reportProgress(u8"整理纹理中");
    m_pTargetSceneInfo->TexImageSet.push_back(Scene::generateBlackPurpleGrid(4, 4, 16));
    m_TexNameToIndex["TextureNotFound"] = 0;

    Scene::SRequestResultFilePath FilePathResult = Scene::requestFilePath("", "", u8"添加纹理", "wad");
    if (FilePathResult.Found)
    {
        CIOGoldsrcWad Wad;
        GoldSrc::readWad(FilePathResult.Data, m_FilePath.parent_path(), Wad);
        m_Wads.emplace_back(std::move(Wad));
    }
}

void CSceneReaderRmf::__readObject(ptr<SRmfObject> vpObject)
{
    if (vpObject->Type.String == "CMapWorld")
    {
        ptr<SRmfWorld> pWorld = std::reinterpret_pointer_cast<SRmfWorld>(vpObject);
        for (ptr<SRmfObject> pObject : pWorld->Objects)
        {
            __readObject(pObject);
        }
    }
    else if (vpObject->Type.String == "CMapGroup")
    {
        ptr<SRmfGroup> pGroup = std::reinterpret_pointer_cast<SRmfGroup>(vpObject);
        for (ptr<SRmfObject> pObject : pGroup->Objects)
        {
            __readObject(pObject);
        }
    }
    else if (vpObject->Type.String == "CMapEntity")
    {
        ptr<SRmfEntity> pEntity = std::reinterpret_pointer_cast<SRmfEntity>(vpObject);
        for (ptr<SRmfObject> pObject : pEntity->Solids)
        {
            __readObject(pObject);
        }
    }
    else if (vpObject->Type.String == "CMapSolid")
    {
        ptr<SRmfSolid> pSolid = std::reinterpret_pointer_cast<SRmfSolid>(vpObject);
        __readSolid(pSolid);
    }
    else
    {
        throw std::runtime_error(u8"rmf对象的类型错误：" + vpObject->Type.String);
    }
}

void CSceneReaderRmf::__readSolid(ptr<SRmfSolid> vpSolid)
{
    CMeshData MeshData = CMeshData();
    for (const SRmfFace& Face : vpSolid->Faces)
        __readSolidFace(Face, MeshData);
    
    // to y-up
    GoldSrc::toYupCounterClockwise(MeshData);

    auto pActor = GoldSrc::createActorByMeshAndTag(MeshData);
    m_pTargetSceneInfo->pScene->addActor(pActor);
}

void CSceneReaderRmf::__readSolidFace(const SRmfFace& vFace, CMeshData& vioMeshData)
{
    uint32_t TexIndex = __requestTextureIndex(vFace.TextureName);

    size_t VertexNum = vFace.Vertices.size();
    std::vector<glm::vec3> Vertices(VertexNum);
    std::vector<glm::vec2> TexCoords(VertexNum);
    for (size_t i = 0; i < VertexNum; ++i)
    {
        Vertices[i] = vFace.Vertices[i].toGlm();
        TexCoords[i] = __getTexCoord(vFace, Vertices[i]);
    }

    glm::vec3 Normal = glm::normalize(glm::cross(Vertices[2] - Vertices[0], Vertices[1] - Vertices[0]));
    
    auto pVertexArray = vioMeshData.getVertexArray();
    auto pColorArray = vioMeshData.getColorArray();
    auto pNormalArray = vioMeshData.getNormalArray();
    auto pTexCoordArray = vioMeshData.getTexCoordArray();
    auto pTexIndexArray = vioMeshData.getTexIndexArray();
    for (size_t k = 2; k < Vertices.size(); ++k)
    {
        pVertexArray->append(Vertices[0] * m_SceneScale);
        pVertexArray->append(Vertices[k - 1] * m_SceneScale);
        pVertexArray->append(Vertices[k] * m_SceneScale);
        pColorArray->append(glm::vec3(1.0, 1.0, 1.0), 3);
        pNormalArray->append(Normal, 3);
        pTexCoordArray->append(TexCoords[0]);
        pTexCoordArray->append(TexCoords[k - 1]);
        pTexCoordArray->append(TexCoords[k]);
        pTexIndexArray->append(TexIndex, 3);
    }
}

uint32_t CSceneReaderRmf::__requestTextureIndex(std::string vTextureName)
{
    if (m_TexNameToIndex.find(vTextureName) != m_TexNameToIndex.end())
        return m_TexNameToIndex.at(vTextureName);
    else
    {
        for (const CIOGoldsrcWad& Wad : m_Wads)
        {
            std::optional<size_t> Index = Wad.findTexture(vTextureName);
            if (Index.has_value())
            {
                ptr<CIOImage> pTexImage = Scene::getIOImageFromWad(Wad, Index.value());
                uint32_t TexIndex = static_cast<uint32_t>(m_pTargetSceneInfo->TexImageSet.size());
                m_TexNameToIndex[vTextureName] = TexIndex;
                m_pTargetSceneInfo->TexImageSet.emplace_back(std::move(pTexImage));
                return TexIndex;
            }
        }
        m_TexNameToIndex[vTextureName] = 0;
        return 0;
    }
}

glm::vec2 CSceneReaderRmf::__getTexCoord(SRmfFace vFace, glm::vec3 vVertex)
{
    _ASSERTE(m_TexNameToIndex.find(vFace.TextureName) != m_TexNameToIndex.end());
    uint32_t TexIndex = m_TexNameToIndex.at(vFace.TextureName);
    const ptr<CIOImage> pImage = m_pTargetSceneInfo->TexImageSet[TexIndex];
    size_t TexWidth = pImage->getWidth();
    size_t TexHeight = pImage->getHeight();

    glm::vec2 TexCoord = {};
    TexCoord.x = (glm::dot(vVertex, vFace.TextureDirectionU.toGlm()) / vFace.TextureScaleU + vFace.TextureOffsetU) / TexWidth;
    TexCoord.y = (glm::dot(vVertex, vFace.TextureDirectionV.toGlm()) / vFace.TextureScaleV + vFace.TextureOffsetV) / TexHeight;
    return TexCoord;
}