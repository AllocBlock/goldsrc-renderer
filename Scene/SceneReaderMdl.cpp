#include "SceneReaderMdl.h"
#include "SceneCommon.h"

std::shared_ptr<SScene> CSceneReaderMdl::_readV()
{
    auto pIOMdl = std::make_shared<CIOGoldSrcMdl>();
    pIOMdl->read(m_FilePath);

    auto pScene = std::make_shared<SScene>();
    auto BodyPartSet = pIOMdl->getBodyParts();
    for (const auto& BodyPart : BodyPartSet)
    {
        auto pObject = __readBodyPart(BodyPart);
        pScene->Objects.emplace_back(pObject);
    }

    uint8_t BaseColor1[3] = { 255, 255, 255 };
    uint8_t BaseColor2[3] = { 255, 0, 255 };
    pScene->TexImages.emplace_back(generateGrid(4, 4, 16, BaseColor1, BaseColor2));

    return pScene;
}

std::shared_ptr<S3DObject> CSceneReaderMdl::__readBodyPart(const SMdlBodyPart& vBodyPart)
{
    auto pObject = std::make_shared<S3DObject>();
    pObject->DataType = E3DObjectDataType::TRIAGNLE_LIST;

    for (const auto& Model : vBodyPart.ModelSet)
    {
        __readModel(Model, pObject);
    }

    return pObject;
}

void CSceneReaderMdl::__readModel(const SMdlModel& vModel, std::shared_ptr<S3DObject> voObject)
{
    size_t VertexStartIndex = voObject->Vertices.size();

    float Scale = 0.0f;
    std::ofstream Log("log.txt", std::ios::out);
    for (const auto& Mesh : vModel.MeshSet)
    {
        for (const auto& TriangleVertex : Mesh.TriangleVertexSet)
        {
            glm::vec3 Vertex = vModel.VertexSet[TriangleVertex.VertexIndex].glmVec3();
            glm::vec3 Normal = vModel.NormalSet[TriangleVertex.NormalIndex].glmVec3();

            //int16_t VertexBoneIndex = vModel.VertexBoneIndexSet[TriangleVertex.VertexIndex];
            //int16_t NormalBoneIndex = vModel.NormalBoneIndexSet[TriangleVertex.NormalIndex];

            // 骨骼变换
            //Vertex = BoneTransform * Vertex;

            Scale = std::max<float>(Scale, std::abs(Vertex.x));
            Scale = std::max<float>(Scale, std::abs(Vertex.y));
            Scale = std::max<float>(Scale, std::abs(Vertex.z));

            voObject->Vertices.emplace_back(Vertex);
            
            Log << Vertex.x << ", " << Vertex.y << ", " << Vertex.z << "\n";
            
            voObject->Normals.emplace_back(Normal);
            voObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            voObject->TexCoords.emplace_back(glm::vec2(0.0, 0.0));
            voObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            voObject->TexIndices.emplace_back(0);
        }
    }
    Log.close();
    
    // 缩放到单位体积内
    for (auto& Vertex : voObject->Vertices)
        Vertex /= Scale;
}