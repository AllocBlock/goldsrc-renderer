#include "SceneReaderMdl.h"
#include "SceneCommon.h"

std::shared_ptr<SScene> CSceneReaderMdl::_readV()
{
    m_pIOMdl = std::make_shared<CIOGoldSrcMdl>();
    m_pIOMdl->read(m_FilePath);

    auto pScene = std::make_shared<SScene>();
    auto BodyPartSet = m_pIOMdl->getBodyParts();
    for (const auto& BodyPart : BodyPartSet)
    {
        auto pObject = __readBodyPart(BodyPart);
        pScene->Objects.emplace_back(pObject);
    }

    auto TextureSet = m_pIOMdl->getTextures();
    for (const SMdlTexture& Texture : TextureSet)
    {
        void* pData = new uint8_t[Texture.Width * Texture.Height * 4];
        Texture.getRawRGBAPixels(pData);
        auto pImage = std::make_shared<CIOImage>();
        pImage->setImageSize(Texture.Width, Texture.Height);
        pImage->setData(pData);
        delete[] pData;

        pScene->TexImages.emplace_back(pImage);
    }

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
    auto TextureSet = m_pIOMdl->getTextures();
    auto SkinReferenceSet = m_pIOMdl->getSkinReferences();

    float Scale = 0.0f;
    for (const auto& Mesh : vModel.MeshSet)
    {
        uint32_t TextureIndex = SkinReferenceSet[Mesh.SkinReference];
        double TextureWidth = TextureSet[TextureIndex].Width;
        double TextureHeight = TextureSet[TextureIndex].Height;
        for (const auto& TriangleVertex : Mesh.TriangleVertexSet)
        {
            glm::vec3 Vertex = vModel.VertexSet[TriangleVertex.VertexIndex].glmVec3();
            glm::vec3 Normal = vModel.NormalSet[TriangleVertex.NormalIndex].glmVec3();
            glm::vec2 TexCoord = glm::vec2(TriangleVertex.S / TextureWidth, TriangleVertex.T / TextureHeight);
           
            //int16_t VertexBoneIndex = vModel.VertexBoneIndexSet[TriangleVertex.VertexIndex];
            //int16_t NormalBoneIndex = vModel.NormalBoneIndexSet[TriangleVertex.NormalIndex];

            // 骨骼变换
            //Vertex = BoneTransform * Vertex;

            Scale = std::max<float>(Scale, std::abs(Vertex.x));
            Scale = std::max<float>(Scale, std::abs(Vertex.y));
            Scale = std::max<float>(Scale, std::abs(Vertex.z));

            voObject->Vertices.emplace_back(Vertex);
            voObject->Normals.emplace_back(Normal);
            voObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            voObject->TexCoords.emplace_back(TexCoord);
            voObject->LightmapCoords.emplace_back(glm::vec2(0.0, 0.0));
            voObject->TexIndices.emplace_back(TextureIndex);
        }
    }

    // 缩放到单位体积内
    for (auto& Vertex : voObject->Vertices)
        Vertex /= Scale;
}