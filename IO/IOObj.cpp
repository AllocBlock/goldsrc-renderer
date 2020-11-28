#include "IOObj.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

bool CIOObj::_readV(std::string vFileName) {
    m_pObj = std::make_shared<SObj>();

    if (!vFileName.empty()) m_FileName = vFileName;
    _ASSERTE(std::filesystem::exists(m_FileName));

    std::ifstream File;
    File.open(m_FileName);
    if (!File.is_open()) {
        GlobalLogger::logStream() << vFileName << " 文件打开失败！" << std::endl;
        return false;
    }

    std::stringstream Line;
    std::string Cmd;
    std::string CurrentGroup = "";
    std::string CurrentSmoothGroup = "";
    std::string CurrentMtl = "";
    char LineBuffer[MAX_LINE_SIZE];
    while (!File.eof()) {
        File.getline(LineBuffer, MAX_LINE_SIZE);
        Line.clear();
        Cmd.clear();
        Line.str(LineBuffer);
        Line >> Cmd;
        if (Cmd.empty() || Cmd[0] == '#') { // 注释
            continue;
        }
        else if (Cmd == "v") { // 顶点
            glm::vec3 Vertex;
            Line >> Vertex.x;
            Line >> Vertex.y;
            Line >> Vertex.z;
            m_pObj->Vertices.push_back(Vertex);
        }
        else if (Cmd == "vt") { // 纹理坐标
            glm::vec2 TexCoor;
            Line >> TexCoor.x;
            Line >> TexCoor.y;
            m_pObj->TexCoors.push_back(TexCoor);
        }
        else if (Cmd == "vn") { // 法向量
            glm::vec3 Normal;
            Line >> Normal.x;
            Line >> Normal.y;
            Line >> Normal.z;
            m_pObj->Normals.push_back(Normal);
        }
        else if (Cmd == "f") { // 面
            SObjFace Face;

            std::string FaceNode;
            std::regex ReFaceNode("(\\d?)/?(\\d?)/?(\\d?)");
            std::smatch ReResult;
            while (true) {
                Line >> FaceNode;
                if (FaceNode.empty()) break;
                if (!std::regex_match(FaceNode, ReResult, ReFaceNode))
                {
                    GlobalLogger::logStream() << " 面数据错误！" << LineBuffer << std::endl;
                    continue;
                }
                SObjFaceNode FaceNodeInfo;

                FaceNodeInfo.VectexId = ReResult[0].str().empty() ? -1 : atoi(ReResult[0].str().c_str());
                FaceNodeInfo.TexCoorIndex = ReResult[1].str().empty() ? -1 : atoi(ReResult[1].str().c_str());
                FaceNodeInfo.NormalIndex = ReResult[2].str().empty() ? -1 : atoi(ReResult[2].str().c_str());

                Face.Nodes.push_back(FaceNodeInfo);
            }
            Face.GroupName = CurrentGroup;
            Face.SmoothGroupName = CurrentSmoothGroup;
            Face.MtlName = CurrentMtl;
            m_pObj->Faces.push_back(Face);
        }
        else if (Cmd == "g") { // 组
            Line >> CurrentGroup;
        }
        else if (Cmd == "mtllib") { // 材质
            std::filesystem::path MtlFileName = vFileName;
            MtlFileName.replace_extension("mtl");
            if (std::filesystem::exists(MtlFileName))
                m_pMtl->read(MtlFileName.string());
        }
        else if (Cmd == "usemtl") { // 材质
            Line >> CurrentMtl;
        }
        else if (Cmd == "s") { // 光滑组
            Line >> CurrentSmoothGroup;
        }
        else {
            GlobalLogger::logStream() << "obj格式有误或不支持的标记：" << Cmd << std::endl;
            continue;
        }
    }

    // 缩放范围到[0, 1]
    float MaxX, MinX, MaxY, MinY, MaxZ, MinZ;
    MinX = MinY = MinZ = INFINITY;
    MaxX = MaxY = MaxZ = -INFINITY;
    for (glm::vec3 Vertex : m_pObj->Vertices) {
        MaxX = std::max<float>(MaxX, Vertex.x);
        MinX = std::min<float>(MinX, Vertex.x);
        MaxY = std::max<float>(MaxY, Vertex.y);
        MinY = std::min<float>(MinY, Vertex.y);
        MaxZ = std::max<float>(MaxZ, Vertex.z);
        MinZ = std::min<float>(MinZ, Vertex.z);;
    }
    m_ScaleFactor = std::max<float>(MaxX - MinX, std::max<float>(MaxY - MinY, MaxZ - MinZ));

    return true;
}

uint32_t CIOObj::getIndex(int faceIndex, int index) {
    return m_pObj->Faces[faceIndex].Nodes[index].VectexId - 1;
}
glm::vec3 CIOObj::getVertex(int faceIndex, int index) {
    return m_pObj->Vertices[m_pObj->Faces[faceIndex].Nodes[index].VectexId - 1] / m_ScaleFactor;
}
glm::vec2 CIOObj::getTexCoor(int faceIndex, int texCoorIndex) {
    int Index = m_pObj->Faces[faceIndex].Nodes[texCoorIndex].TexCoorIndex;
    if (Index == -1) Index = 0;
    else Index--;
    // 纹理映射修改，obj->vulkan
    glm::vec2 texCoor = m_pObj->TexCoors[Index];
    texCoor.y = 1.0 - texCoor.y;
    //return texCoor;
    return m_pObj->TexCoors[0];
}
glm::vec3 CIOObj::getNormal(int faceIndex, int normalIndex) {
    /*uint32_t index = m_pObj->Faces[faceIndex].Nodes[normalIndex].normalIndex;
    if (index == uint32_t(-1)) index = 0;
    else index--;
    return normalsList[index];*/


    glm::vec3 vert = getVertex(faceIndex, normalIndex);
    glm::vec3 normal = glm::vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < m_pObj->Faces.size(); i++) {
        for (int j = 0; j < m_pObj->Faces[i].Nodes.size(); j++) {
            if (vert == getVertex(i, j)) {
                glm::vec3 p1 = getVertex(i, 0);
                glm::vec3 p2 = getVertex(i, 1);
                glm::vec3 p3 = getVertex(i, 2);
                normal += normalize(cross(p2 - p1, p3 - p1));
                break;
            }
        }
    }
    return normalize(normal);
}