#include "SceneReaderRmf.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

SScene CSceneReaderRmf::read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    m_ProgressReportFunc = vProgressReportFunc;
    m_Scene = SScene();
    
    __readRmf(vFilePath);
    __readWadsAndInitTextures();
    __reportProgress(u8"读取场景中");
    __readObject(m_Rmf.getWorld());

    return m_Scene;
}

void CSceneReaderRmf::__readRmf(std::filesystem::path vFilePath)
{
    __reportProgress(u8"[rmf]读取文件中");
    m_Rmf = CIOGoldSrcRmf(vFilePath);
    if (!m_Rmf.read())
        throw std::runtime_error(u8"文件解析失败");
}

void CSceneReaderRmf::__readWadsAndInitTextures()
{
    // read wads
    __reportProgress(u8"初始化纹理中");
    m_TexNameToIndex.clear();
    m_TexNameToIndex["TextureNotFound"] = 0;
    m_Scene.TexImages.emplace_back(generateBlackPurpleGrid(4, 4, 16));

    __reportProgress(u8"rmf不包含wad信息，将来会加入wad手动选择");
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
    auto pObject = std::make_shared<S3DObject>();
    for (const SRmfFace& Face : vpSolid->Faces)
        __readSolidFace(Face, pObject);

    m_Scene.Objects.emplace_back(std::move(pObject));
}

void CSceneReaderRmf::__readSolidFace(const SRmfFace& vFace, std::shared_ptr<S3DObject> vopObject)
{
    uint32_t TexIndex = __requestTextureIndex(vFace.TextureName);

    size_t VertexNum = vFace.Vertices.size();
    std::vector<glm::vec3> Vertices(VertexNum);
    std::vector<glm::vec2> TexCoords(VertexNum);
    for (size_t i = 0; i < VertexNum; ++i)
    {
        Vertices[i] = vFace.Vertices[i].glmVec3();
        TexCoords[i] = __getTexCoord(vFace, Vertices[i]);
    }

    glm::vec3 Normal = glm::normalize(glm::cross(Vertices[2] - Vertices[0], Vertices[1] - Vertices[0]));
    
    for (size_t k = 2; k < Vertices.size(); ++k)
    {
        vopObject->Vertices.emplace_back(Vertices[0] * m_SceneScale);
        vopObject->Vertices.emplace_back(Vertices[k - 1] * m_SceneScale);
        vopObject->Vertices.emplace_back(Vertices[k] * m_SceneScale);
        vopObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        vopObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        vopObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        vopObject->Normals.emplace_back(Normal);
        vopObject->Normals.emplace_back(Normal);
        vopObject->Normals.emplace_back(Normal);
        vopObject->TexCoords.emplace_back(TexCoords[0]);
        vopObject->TexCoords.emplace_back(TexCoords[k - 1]);
        vopObject->TexCoords.emplace_back(TexCoords[k]);
        vopObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
        vopObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
        vopObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
        vopObject->TexIndices.emplace_back(TexIndex);
        vopObject->TexIndices.emplace_back(TexIndex);
        vopObject->TexIndices.emplace_back(TexIndex);
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
                std::shared_ptr<CIOImage> pTexImage = getIOImageFromWad(Wad, Index.value());
                uint32_t TexIndex = m_Scene.TexImages.size();
                m_TexNameToIndex[vTextureName] = TexIndex;
                m_Scene.TexImages.emplace_back(std::move(pTexImage));
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
    const std::shared_ptr<CIOImage> pImage = m_Scene.TexImages[TexIndex];
    size_t TexWidth = pImage->getImageWidth();
    size_t TexHeight = pImage->getImageHeight();

    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex, vFace.TextureDirectionU.glmVec3()) / vFace.TextureScaleU + vFace.TextureOffsetU) / TexWidth;
    TexCoord.y = (glm::dot(vVertex, vFace.TextureDirectionV.glmVec3()) / vFace.TextureScaleV + vFace.TextureOffsetV) / TexHeight;
    return TexCoord;
}