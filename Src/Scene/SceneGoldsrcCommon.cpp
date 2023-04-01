#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "Log.h"
#include "ComponentMeshRenderer.h"

CActor::Ptr GoldSrc::createActorByMeshAndTag(const CMeshData& vMeshData, const std::vector<std::string> vTagSet)
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
    auto RequestResult = Scene::requestFilePath(vWadPath, vAdditionalSearchDir, u8"��Ҫ����" + vWadPath.filename().u8string(), "wad");
    if (!RequestResult.Found)
    {
        Log::log("δ�ҵ�WAD�ļ���" + vWadPath.u8string());
        return false;
    }

    Scene::reportProgress(u8"[wad]��ȡ" + RequestResult.Data.u8string() + u8"�ļ���");
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