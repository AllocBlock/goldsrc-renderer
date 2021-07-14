#include "SceneReaderRmf.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

using namespace Common;

std::shared_ptr<SScene> CSceneReaderRmf::_readV()
{
    m_pScene = std::make_shared<SScene>();
    
    __readRmf(m_FilePath);
    __readWadsAndInitTextures();
    Scene::reportProgress(u8"读取场景中");
    __readObject(m_Rmf.getWorld());

    return m_pScene;
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
    m_TexNameToIndex["TextureNotFound"] = 0;
    m_pScene->TexImages.emplace_back(Scene::generateBlackPurpleGrid(4, 4, 16));

    Scene::reportProgress(u8"rmf不包含wad信息，将来会加入wad手动选择");
    // TODO: provide wads file selection for user
}

void CSceneReaderRmf::__readObject(std::shared_ptr<SRmfObject> vpObject)
{
    if (vpObject->Type.String == "CMapWorld")
    {
        std::shared_ptr<SRmfWorld> pWorld = std::reinterpret_pointer_cast<SRmfWorld>(vpObject);
        for (std::shared_ptr<SRmfObject> pObject : pWorld->Objects)
        {
            __readObject(pObject);
        }
    }
    else if (vpObject->Type.String == "CMapGroup")
    {
        std::shared_ptr<SRmfGroup> pGroup = std::reinterpret_pointer_cast<SRmfGroup>(vpObject);
        for (std::shared_ptr<SRmfObject> pObject : pGroup->Objects)
        {
            __readObject(pObject);
        }
    }
    else if (vpObject->Type.String == "CMapEntity")
    {
        std::shared_ptr<SRmfEntity> pEntity = std::reinterpret_pointer_cast<SRmfEntity>(vpObject);
        for (std::shared_ptr<SRmfObject> pObject : pEntity->Solids)
        {
            __readObject(pObject);
        }
    }
    else if (vpObject->Type.String == "CMapSolid")
    {
        std::shared_ptr<SRmfSolid> pSolid = std::reinterpret_pointer_cast<SRmfSolid>(vpObject);
        __readSolid(pSolid);
    }
    else
    {
        throw std::runtime_error(u8"rmf对象的类型错误：" + vpObject->Type.String);
    }
}

void CSceneReaderRmf::__readSolid(std::shared_ptr<SRmfSolid> vpSolid)
{
    auto pObject = std::make_shared<C3DObjectGoldSrc>();
    for (const SRmfFace& Face : vpSolid->Faces)
        __readSolidFace(Face, pObject);

    m_pScene->Objects.emplace_back(std::move(pObject));
}

void CSceneReaderRmf::__readSolidFace(const SRmfFace& vFace, std::shared_ptr<C3DObjectGoldSrc> vopObject)
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
    
    auto pVertexArray = vopObject->getVertexArray();
    auto pColorArray = vopObject->getColorArray();
    auto pNormalArray = vopObject->getNormalArray();
    auto pTexCoordArray = vopObject->getTexCoordArray();
    auto pLightmapCoordArray = vopObject->getLightmapCoordArray();
    auto pTexIndexArray = vopObject->getTexIndexArray();
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
        pLightmapCoordArray->append(glm::vec2(0.0, 0.0), 3);
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
                std::shared_ptr<CIOImage> pTexImage = Scene::getIOImageFromWad(Wad, Index.value());
                uint32_t TexIndex = static_cast<uint32_t>(m_pScene->TexImages.size());
                m_TexNameToIndex[vTextureName] = TexIndex;
                m_pScene->TexImages.emplace_back(std::move(pTexImage));
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
    const std::shared_ptr<CIOImage> pImage = m_pScene->TexImages[TexIndex];
    size_t TexWidth = pImage->getWidth();
    size_t TexHeight = pImage->getHeight();

    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex, vFace.TextureDirectionU.toGlm()) / vFace.TextureScaleU + vFace.TextureOffsetU) / TexWidth;
    TexCoord.y = (glm::dot(vVertex, vFace.TextureDirectionV.toGlm()) / vFace.TextureScaleV + vFace.TextureOffsetV) / TexHeight;
    return TexCoord;
}