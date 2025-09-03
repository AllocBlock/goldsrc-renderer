#include "SceneReaderObj.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "IOObj.h"

void CSceneReaderObj::_readV(sptr<SSceneInfo> voSceneInfo)
{
    Scene::reportProgress(u8"[obj]读取文件中");
    CIOObj Obj = CIOObj();
    Obj.read(m_FilePath);

    Scene::reportProgress(u8"读取场景中");
    CMeshData MeshData = CMeshData();
    const uint32_t TexIndex = 0;

    auto pVertexArray = MeshData.getVertexArray();
    auto pColorArray = MeshData.getColorArray();
    auto pNormalArray = MeshData.getNormalArray();
    auto pTexCoordArray = MeshData.getTexCoordArray();
    auto pTexIndexArray = MeshData.getTexIndexArray();

    const std::vector<SObjFace>& Faces = Obj.getFaces();
    for (size_t i = 0; i < Faces.size(); ++i)
    {
        const SObjFace& Face = Faces[i];
        size_t NodeNum = Face.Nodes.size();
        if (NodeNum == 0) continue;

        std::vector<glm::vec3> Vertices(NodeNum);
        std::vector<glm::vec3> Normals(NodeNum);
        std::vector<glm::vec2> TexCoords(NodeNum);

        // TODO: calc face normal if no normal in file
        for (size_t k = 0; k < NodeNum; ++k)
        {
            Obj.dumpVertex(i, k, Vertices[k]);
            if (!Obj.dumpNormal(i, k, Normals[k]))
                Normals[k] = glm::vec3(0, 0, 0);
            if (!Obj.dumpTexCoord(i, k, TexCoords[k]))
                TexCoords[k] = glm::vec2(0, 0);
        }

        // FIXME: concave polygon should not use this triangulation
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
            pTexIndexArray->append(TexIndex, 3);
        }
    }

    auto pActor = GoldSrc::createActorByMeshAndTag(MeshData);
    
    voSceneInfo->pScene->addActor(pActor);
    voSceneInfo->TexImageSet.emplace_back(Scene::generateBlackPurpleGrid(4, 4, 16));
}