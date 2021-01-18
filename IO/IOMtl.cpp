#include "IOMtl.h"

#include <sstream>

bool CIOMtl::_readV(std::filesystem::path vFilePath) {
    std::ifstream File;
    File.open(vFilePath);
    if (!File.is_open()) {
        GlobalLogger::logStream() << vFilePath << u8" 文件打开失败";
        return false;
    }

    SMtlMaterial Material;
    while (true) {
        std::string Cmd;
        if (File.eof()) {
            m_Materials.push_back(Material);
            break;
        }
        std::stringstream Line; // 放在循环内部，保证每次都会清空
        char LineBuffer[MAX_LINE_SIZE];
        File.getline(LineBuffer, MAX_LINE_SIZE);
        Line.clear();
        Cmd.clear();
        Line.str(LineBuffer);
        Line >> Cmd;
        if (Cmd.empty() || Cmd[0] == '#') { // 注释
            continue;
        }
        else if (Cmd == "newmtl") { // 面
            if (!Material.Name.empty()) m_Materials.push_back(Material);
            Material = SMtlMaterial();
            Line >> Material.Name;
        }
        else if (Cmd == "Ka") {
            Line >> Material.Ka.r;
            Line >> Material.Ka.g;
            Line >> Material.Ka.b;
        }
        else if (Cmd == "Kd") {
            Line >> Material.Kd.r;
            Line >> Material.Kd.g;
            Line >> Material.Kd.b;
        }
        else if (Cmd == "Ks") {
            Line >> Material.Ks.r;
            Line >> Material.Ks.g;
            Line >> Material.Ks.b;
        }
        else if (Cmd == "Ke") {
            Line >> Material.Ke.r;
            Line >> Material.Ke.g;
            Line >> Material.Ke.b;
        }
        else if (Cmd == "d")  Line >> Material.D;
        else if (Cmd == "Tr")  Line >> Material.Tr;
        else if (Cmd == "Tf")  Line >> Material.Tf;
        else if (Cmd == "Ns")  Line >> Material.Ns;
        else if (Cmd == "Ni")  Line >> Material.Ni;
        else if (Cmd == "illum")  Line >> Material.IlluMode;
        else if (Cmd == "map_Ka")  Line >> Material.Map_Ka;
        else if (Cmd == "map_Kd")  Line >> Material.Map_Kd;
        else if (Cmd == "map_bump")  Line >> Material.Map_bump;
        else if (Cmd == "bump")  Line >> Material.Bump;
        else {
            GlobalLogger::logStream() << m_FilePath.u8string() << u8" mtl格式有误或不支持：" << Cmd;
            continue;
        }
    }
    return true;
}
