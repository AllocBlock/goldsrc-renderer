#include "SceneReaderRmf.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

using namespace Common;

ptr<SScene> CSceneReaderRmf::_readV()
{
    m_pScene = make<SScene>();
    
    __readRmf(m_FilePath);
    __readWadsAndInitTextures();
    Scene::reportProgress(u8"��ȡ������");
    __readObject(m_Rmf.getWorld());

    return m_pScene;
}

void CSceneReaderRmf::__readRmf(std::filesystem::path vFilePath)
{
    Scene::reportProgress(u8"[rmf]��ȡ�ļ���");
    m_Rmf = CIOGoldSrcRmf(vFilePath);
    if (!m_Rmf.read())
        throw std::runtime_error(u8"�ļ�����ʧ��");
}

void CSceneReaderRmf::__readWadsAndInitTextures()
{
    // read wads
    Scene::reportProgress(u8"��ʼ��������");
    m_TexNameToIndex.clear();
    
    Scene::reportProgress(u8"����������");
    m_pScene->TexImageSet.push_back(Scene::generateBlackPurpleGrid(4, 4, 16));
    m_TexNameToIndex["TextureNotFound"] = 0;
    while (true)
    {
        Scene::SRequestResultFilePath FilePathResult;
        FilePathResult = Scene::requestFilePath(u8"��������", ".wad");
        if (FilePathResult.State == Scene::ERequestResultState::CONTINUE)
        {
            CIOGoldsrcWad Wad;
            Common::GoldSrc::readWad(FilePathResult.Data, m_FilePath.parent_path(), Wad);
            m_Wads.emplace_back(std::move(Wad));
        }
        else
        {
            break;
        }
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
        throw std::runtime_error(u8"rmf��������ʹ���" + vpObject->Type.String);
    }
}

void CSceneReaderRmf::__readSolid(ptr<SRmfSolid> vpSolid)
{
    auto pObject = make<C3DObjectGoldSrc>();
    for (const SRmfFace& Face : vpSolid->Faces)
        __readSolidFace(Face, pObject);

    m_pScene->Objects.emplace_back(std::move(pObject));
}

void CSceneReaderRmf::__readSolidFace(const SRmfFace& vFace, ptr<C3DObjectGoldSrc> vopObject)
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
                ptr<CIOImage> pTexImage = Scene::getIOImageFromWad(Wad, Index.value());
                uint32_t TexIndex = static_cast<uint32_t>(m_pScene->TexImageSet.size());
                m_TexNameToIndex[vTextureName] = TexIndex;
                m_pScene->TexImageSet.emplace_back(std::move(pTexImage));
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
    const ptr<CIOImage> pImage = m_pScene->TexImageSet[TexIndex];
    size_t TexWidth = pImage->getWidth();
    size_t TexHeight = pImage->getHeight();

    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex, vFace.TextureDirectionU.toGlm()) / vFace.TextureScaleU + vFace.TextureOffsetU) / TexWidth;
    TexCoord.y = (glm::dot(vVertex, vFace.TextureDirectionV.toGlm()) / vFace.TextureScaleV + vFace.TextureOffsetV) / TexHeight;
    return TexCoord;
}