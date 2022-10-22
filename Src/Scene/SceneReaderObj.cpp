#include "SceneReaderObj.h"
#include "SceneCommon.h"
#include "IOObj.h"

ptr<SScene> CSceneReaderObj::_readV()
{
    Scene::reportProgress(u8"[obj]读取文件中");
    CIOObj Obj = CIOObj();
    Obj.read(m_FilePath);

    Scene::reportProgress(u8"读取场景中");
    ptr<CMeshDataGoldSrc> pObjObject = make<CMeshDataGoldSrc>();
    const uint32_t TexIndex = 0;

    auto pVertexArray = pObjObject->getVertexArray();
    auto pColorArray = pObjObject->getColorArray();
    auto pNormalArray = pObjObject->getNormalArray();
    auto pTexCoordArray = pObjObject->getTexCoordArray();
    auto pLightmapCoordArray = pObjObject->getLightmapCoordArray();
    auto pTexIndexArray = pObjObject->getTexIndexArray();

    const std::vector<SObjFace>& Faces = Obj.getFaces();
    for (size_t i = 0; i < Faces.size(); ++i)
    {
        const SObjFace& Face = Faces[i];
        size_t NodeNum = Face.Nodes.size();
        if (NodeNum == 0) continue;

        std::vector<glm::vec3> Vertices(NodeNum);
        std::vector<glm::vec3> Normals(NodeNum);
        std::vector<glm::vec2> TexCoords(NodeNum);

        // get data backwards to form clockwise points
        for (size_t k = 0; k < NodeNum; ++k)
        {
            Vertices.emplace_back(Obj.getVertex(i, NodeNum - k - 1));
            Normals.emplace_back(Obj.getNormal(i, NodeNum - k - 1));
            TexCoords.emplace_back(Obj.getTexCoord(i, NodeNum - k - 1));
        }

        for (size_t k = 2; k < Face.Nodes.size(); ++k)
        {
            pVertexArray->append(Vertices[0]);
            pVertexArray->append(Vertices[k - 1]);
            pVertexArray->append(Vertices[k]);
            pColorArray->append(glm::vec3(1.0, 1.0, 1.0), 3);
            pNormalArray->append(Normals[0]);
            pNormalArray->append(Normals[k - 1]);
            pNormalArray->append(Normals[k]);
            pTexCoordArray->append(TexCoords[0]);
            pTexCoordArray->append(TexCoords[k - 1]);
            pTexCoordArray->append(TexCoords[k]);
            pLightmapCoordArray->append(glm::vec2(0.0, 0.0), 3);
            pTexIndexArray->append(TexIndex, 3);
        }
    }

    m_pScene = make<SScene>();
    m_pScene->Objects.emplace_back(pObjObject);
    m_pScene->TexImageSet.emplace_back(Scene::generateBlackPurpleGrid(4, 4, 16));

    return m_pScene;
}