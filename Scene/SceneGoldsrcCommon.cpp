#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

using namespace Common;

std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths)
{
    std::vector<CIOGoldsrcWad> Wads(vWadPaths.size());

    for (size_t i = 0; i < vWadPaths.size(); ++i)
    {
        std::filesystem::path RealWadPath;
        if (!Scene::findFile(vWadPaths[i], "../data", RealWadPath))
        {
            Common::Log::log(u8"δ�ҵ�WAD�ļ���" + vWadPaths[i].u8string());
            continue;
        }
        Scene::reportProgress(u8"[wad]��ȡ" + RealWadPath.u8string() + u8"�ļ���");
        Wads[i].read(RealWadPath);
    }
    return Wads;
}