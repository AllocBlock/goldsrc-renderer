#include "SceneReaderObj.h"
#include "SceneCommon.h"
#include "IOObj.h"

std::shared_ptr<SScene> CSceneReaderObj::read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    m_ProgressReportFunc = vProgressReportFunc;
    
    __reportProgress(u8"[obj]读取文件中");
    CIOObj Obj = CIOObj();
    Obj.read(vFilePath);

    __reportProgress(u8"读取场景中");
    std::shared_ptr<S3DObject> pObjObject = std::make_shared<S3DObject>();
    const uint32_t TexIndex = 0;

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
            pObjObject->Vertices.emplace_back(Vertices[0]);
            pObjObject->Vertices.emplace_back(Vertices[k - 1]);
            pObjObject->Vertices.emplace_back(Vertices[k]);
            pObjObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObjObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObjObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObjObject->Normals.emplace_back(Normals[0]);
            pObjObject->Normals.emplace_back(Normals[k - 1]);
            pObjObject->Normals.emplace_back(Normals[k]);
            pObjObject->TexCoords.emplace_back(TexCoords[0]);
            pObjObject->TexCoords.emplace_back(TexCoords[k - 1]);
            pObjObject->TexCoords.emplace_back(TexCoords[k]);
            pObjObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObjObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObjObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            pObjObject->TexIndices.emplace_back(TexIndex);
            pObjObject->TexIndices.emplace_back(TexIndex);
            pObjObject->TexIndices.emplace_back(TexIndex);
        }
    }

    m_pScene = std::make_shared<SScene>();
    m_pScene->Objects.emplace_back(pObjObject);
    m_pScene->TexImages.emplace_back(generateBlackPurpleGrid(4, 4, 16));

    return m_pScene;
}