#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "Log.h"
#include "ComponentMeshRenderer.h"

sptr<CActor> GoldSrc::createActorByMeshAndTag(const CMeshData& vMeshData, const std::vector<std::string> vTagSet)
{
    auto pActor = make<CActor>();
    auto pMesh = make<CMeshTriangleList>();

    pMesh->setMeshData(vMeshData);

    auto pMeshRenderer = make<CComponentMeshRenderer>();
    pMeshRenderer->setMesh(pMesh);

    pActor->getTransform()->addComponent(pMeshRenderer);

    for (const auto& vTag : vTagSet)
        pActor->addTag(vTag);

    return pActor;
}

bool GoldSrc::readWad(std::filesystem::path vWadPath, std::filesystem::path vAdditionalSearchDir, CIOGoldsrcWad& voWad)
{
    auto RequestResult = Scene::requestFilePath(vWadPath, vAdditionalSearchDir, u8"需要纹理：" + vWadPath.filename().u8string(), "wad");
    if (!RequestResult.Found)
    {
        Log::log("未找到WAD文件：" + vWadPath.u8string());
        return false;
    }

    Scene::reportProgress(u8"[wad]读取" + RequestResult.Data.u8string() + u8"文件中");
    voWad.read(RequestResult.Data);

    return true;
}

std::vector<CIOGoldsrcWad> GoldSrc::readWads(const std::vector<std::filesystem::path>& vWadPaths, std::filesystem::path vAdditionalSearchDir)
{
    std::vector<CIOGoldsrcWad> Wads(vWadPaths.size());

    for (size_t i = 0; i < vWadPaths.size(); ++i)
    {
        readWad(vWadPaths[i], vAdditionalSearchDir, Wads[i]);
    }
    return Wads;
}

glm::vec3 GoldSrc::toYup(const glm::vec3& vPoint)
{
    // x, y, z -> x, z, -y
    return glm::vec3(vPoint[0], vPoint[2], -vPoint[1]);
}

void GoldSrc::toYupCounterClockwise(const CMeshData& vMeshData)
{
    auto pVertexArray = vMeshData.getVertexArray();
    auto pColorArray = vMeshData.getColorArray();
    auto pNormalArray = vMeshData.getNormalArray();
    auto pTexCoordArray = vMeshData.getTexCoordArray();
    auto pTexIndexArray = vMeshData.getTexIndexArray();
    auto pLightmapIndexArray = vMeshData.getLightmapIndexArray();
    auto pLightmapCoordArray = vMeshData.getLightmapTexCoordArray();
    _ASSERTE(pVertexArray->size() % 3 == 0);
    for (size_t i = 0; i < pVertexArray->size(); i += 3)
    {
        glm::vec3 A = toYup(pVertexArray->get(i));
        glm::vec3 B = toYup(pVertexArray->get(i + 1));
        glm::vec3 C = toYup(pVertexArray->get(i + 2));

        pVertexArray->set(i, A);
        pVertexArray->set(i + 1, C);
        pVertexArray->set(i + 2, B);
        
        if (!pNormalArray->empty())
        {
            glm::vec3 NA = toYup(pNormalArray->get(i));
            glm::vec3 NB = toYup(pNormalArray->get(i + 1));
            glm::vec3 NC = toYup(pNormalArray->get(i + 2));
            pNormalArray->set(i, NA);
            pNormalArray->set(i + 1, NC);
            pNormalArray->set(i + 2, NB);
        }

        // FIXME: bad copy as no reflection
        if (!pColorArray->empty())
        {
            auto Temp = pColorArray->get(i + 1);
            pColorArray->set(i + 1, pColorArray->get(i + 2));
            pColorArray->set(i + 2, Temp);
        }
        if (!pTexCoordArray->empty())
        {
            auto Temp = pTexCoordArray->get(i + 1);
            pTexCoordArray->set(i + 1, pTexCoordArray->get(i + 2));
            pTexCoordArray->set(i + 2, Temp);
        }
        if (!pTexIndexArray->empty())
        {
            auto Temp = pTexIndexArray->get(i + 1);
            pTexIndexArray->set(i + 1, pTexIndexArray->get(i + 2));
            pTexIndexArray->set(i + 2, Temp);
        }
        if (!pLightmapIndexArray->empty())
        {
            auto Temp = pLightmapIndexArray->get(i + 1);
            pLightmapIndexArray->set(i + 1, pLightmapIndexArray->get(i + 2));
            pLightmapIndexArray->set(i + 2, Temp);
        }
        if (!pLightmapCoordArray->empty())
        {
            auto Temp = pLightmapCoordArray->get(i + 1);
            pLightmapCoordArray->set(i + 1, pLightmapCoordArray->get(i + 2));
            pLightmapCoordArray->set(i + 2, Temp);
        }
    }
}