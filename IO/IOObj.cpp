#include "IOObj.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <random>

bool CIOObj::_readV(std::filesystem::path vFilePath) {
    m_pObj = std::make_shared<SObj>();

    if (!vFilePath.empty()) m_FilePath = vFilePath;
    _ASSERTE(std::filesystem::exists(m_FilePath));

    std::ifstream File;
    File.open(m_FilePath);
    if (!File.is_open()) {
        GlobalLogger::logStream() << vFilePath.u8string() << u8" 文件打开失败";
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
            m_pObj->TexCoords.push_back(TexCoor);
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
            std::regex ReFaceNode("(\\d*)/?(\\d*)/?(\\d*)");
            std::smatch ReResult;
            while (true) {
                FaceNode.clear();
                Line >> FaceNode;
                if (FaceNode.empty()) break;
                if (!std::regex_match(FaceNode, ReResult, ReFaceNode))
                {
                    GlobalLogger::logStream() << u8" 面数据错误" << LineBuffer;
                    continue;
                }
                SObjFaceNode FaceNodeInfo;

                FaceNodeInfo.VectexId = ReResult[1].str().empty() ? 0 : atoi(ReResult[1].str().c_str());
                FaceNodeInfo.TexCoordId = ReResult[2].str().empty() ? 0 : atoi(ReResult[2].str().c_str());
                FaceNodeInfo.NormalId = ReResult[3].str().empty() ? 0 : atoi(ReResult[3].str().c_str());

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
            std::filesystem::path MtlFilePath = vFilePath;
            MtlFilePath.replace_extension("mtl");
            if (std::filesystem::exists(MtlFilePath))
            {
                m_pMtl = std::make_shared<CIOMtl>();
                m_pMtl->read(MtlFilePath.string());
            }
        }
        else if (Cmd == "usemtl") { // 材质
            Line >> CurrentMtl;
        }
        else if (Cmd == "s") { // 光滑组
            Line >> CurrentSmoothGroup;
        }
        else {
            GlobalLogger::logStream() << m_FilePath.u8string() << u8" obj格式有误或不支持的标记：" << Cmd;
            continue;
        }
    }

    // 缩放范围到[0, 1]
    float MaxX, MinX, MaxY, MinY, MaxZ, MinZ;
    MinX = MinY = MinZ = INFINITY;
    MaxX = MaxY = MaxZ = -INFINITY;
    for (glm::vec3 Vertex : m_pObj->Vertices)
    {
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

size_t CIOObj::getFaceNum() const
{ 
    if (m_pObj) return m_pObj->Faces.size();
    else return 0;
}

size_t CIOObj::getFaceNodeNum(int vFaceIndex) const
{
    if (!m_pObj || vFaceIndex < 0 || vFaceIndex >= getFaceNum()) return 0;
    return m_pObj->Faces[vFaceIndex].Nodes.size();
}

glm::vec3 CIOObj::getVertex(int vFaceIndex, int vNodeIndex) const
{
    if (!m_pObj || 
        vFaceIndex < 0 || vFaceIndex >= getFaceNum() ||
        vNodeIndex < 0 || vNodeIndex >= m_pObj->Faces[vFaceIndex].Nodes.size())
        return glm::vec3(NAN, NAN, NAN);
    uint32_t VertexId = m_pObj->Faces[vFaceIndex].Nodes[vNodeIndex].VectexId;
    return m_pObj->Vertices[VertexId - 1] / m_ScaleFactor;
}

glm::vec2 CIOObj::getTexCoord(int vFaceIndex, int vNodeIndex) const
{
    if (!m_pObj ||
        vFaceIndex < 0 || vFaceIndex >= getFaceNum() ||
        vNodeIndex < 0 || vNodeIndex >= m_pObj->Faces[vFaceIndex].Nodes.size())
        return glm::vec2(NAN, NAN);
    uint32_t TexCoordId = m_pObj->Faces[vFaceIndex].Nodes[vNodeIndex].TexCoordId;
    if (TexCoordId == 0)
        return glm::vec2(NAN, NAN);
    // 纹理映射方式不同，obj->vulkan
    glm::vec2 TexCoor = m_pObj->TexCoords[TexCoordId - 1];
    TexCoor.y = 1.0 - TexCoor.y;
    return TexCoor;
}

glm::vec3 CIOObj::getNormal(int vFaceIndex, int vNodeIndex) const
{
    if (!m_pObj ||
        vFaceIndex < 0 || vFaceIndex >= getFaceNum() ||
        vNodeIndex < 0 || vNodeIndex >= m_pObj->Faces[vFaceIndex].Nodes.size())
        return glm::vec3(NAN, NAN, NAN);
    uint32_t NormalId = m_pObj->Faces[vFaceIndex].Nodes[vNodeIndex].NormalId;
    if (NormalId == 0)
        return glm::vec3(NAN, NAN, NAN);
    return m_pObj->Normals[NormalId - 1];
}