#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths, std::function<void(std::string)> vProgressReportFunc)
{
    std::vector<CIOGoldsrcWad> Wads(vWadPaths.size());

    for (size_t i = 0; i < vWadPaths.size(); ++i)
    {
        std::filesystem::path RealWadPath;
        if (!findFile(vWadPaths[i], "../data", RealWadPath))
            GlobalLogger::logStream() << u8"δ�ҵ����޷���WAD�ļ���" << vWadPaths[i];
        if (vProgressReportFunc) vProgressReportFunc(u8"[wad]��ȡ" + RealWadPath.u8string() + u8"�ļ���");
        Wads[i].read(RealWadPath);
    }
    return Wads;
}