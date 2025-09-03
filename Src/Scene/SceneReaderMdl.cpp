#include "SceneReaderMdl.h"
#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

void CSceneReaderMdl::_readV(sptr<SSceneInfo> voSceneInfo)
{
    m_pIOMdl = make<CIOGoldSrcMdl>();
    m_pIOMdl->read(m_FilePath);
    
    auto BodyPartSet = m_pIOMdl->getBodyParts();
    for (const auto& BodyPart : BodyPartSet)
    {
        auto pObject = __readBodyPart(BodyPart);
        voSceneInfo->pScene->addActor(pObject);
    }

    auto TextureSet = m_pIOMdl->getTextures();
    for (const SMdlTexture& Texture : TextureSet)
    {
        void* pData = new uint8_t[Texture.Width * Texture.Height * 4];
        Texture.getRawRGBAPixels(pData);
        auto pImage = make<CIOImage>();
        pImage->setSize(Texture.Width, Texture.Height);
        pImage->setData(pData);
        delete[] pData;

        voSceneInfo->TexImageSet.emplace_back(pImage);
    }
}

sptr<CActor> CSceneReaderMdl::__readBodyPart(const SMdlBodyPart& vBodyPart)
{
    CMeshData MeshData;

    for (const auto& Model : vBodyPart.ModelSet)
    {
        __appendModelData(Model, MeshData);
    }
    
    // to y-up
    GoldSrc::toYupCounterClockwise(MeshData);

    return GoldSrc::createActorByMeshAndTag(MeshData, { "model" });
}

void CSceneReaderMdl::__appendModelData(const SMdlModel& vModel, CMeshData& vioMeshData)
{
    auto pVertexArray = vioMeshData.getVertexArray();
    auto pColorArray = vioMeshData.getColorArray();
    auto pNormalArray = vioMeshData.getNormalArray();
    auto pTexCoordArray = vioMeshData.getTexCoordArray();
    auto pTexIndexArray = vioMeshData.getTexIndexArray();

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
            glm::vec3 Vertex = vModel.VertexSet[TriangleVertex.VertexIndex].toGlm();
            glm::vec3 Normal = vModel.NormalSet[TriangleVertex.NormalIndex].toGlm();
            glm::vec2 TexCoord = glm::vec2(TriangleVertex.S / TextureWidth, TriangleVertex.T / TextureHeight);
           
            //int16_t VertexBoneIndex = vModel.VertexBoneIndexSet[TriangleVertex.VertexIndex];
            //int16_t NormalBoneIndex = vModel.NormalBoneIndexSet[TriangleVertex.NormalIndex];

            // �����任
            //Vertex = BoneTransform * Vertex;

            Scale = std::max<float>(Scale, std::abs(Vertex.x));
            Scale = std::max<float>(Scale, std::abs(Vertex.y));
            Scale = std::max<float>(Scale, std::abs(Vertex.z));

            pVertexArray->append(Vertex);
            pColorArray->append(Normal);
            pNormalArray->append(glm::vec3(1.0, 1.0, 1.0));
            pTexCoordArray->append(TexCoord);
            pTexIndexArray->append(TextureIndex);
        }
    }

    // ���ŵ���λ�����
    for (size_t i = 0; i < pVertexArray->size(); ++i)
        pVertexArray->set(i, pVertexArray->get(i) / Scale);
}