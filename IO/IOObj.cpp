#include "IOObj.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <random>

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
                    GlobalLogger::logStream() << " 面数据错误！" << LineBuffer << std::endl;
                    continue;
                }
                SObjFaceNode FaceNodeInfo;

                FaceNodeInfo.VectexId = ReResult[1].str().empty() ? 0 : atoi(ReResult[0].str().c_str());
                FaceNodeInfo.TexCoorId = ReResult[2].str().empty() ? 0 : atoi(ReResult[1].str().c_str());
                FaceNodeInfo.NormalId = ReResult[3].str().empty() ? 0 : atoi(ReResult[2].str().c_str());

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
    uint32_t TexCoorId = m_pObj->Faces[vFaceIndex].Nodes[vNodeIndex].TexCoorId;
    if (TexCoorId == 0)
        return glm::vec2(NAN, NAN);
    // 纹理映射方式不同，obj->vulkan
    glm::vec2 TexCoor = m_pObj->TexCoords[TexCoorId - 1];
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

std::vector<glm::vec3> CIOObj::getNormalPerVertex()
{
    size_t NumVertex = m_pObj->Vertices.size();

    std::vector<glm::vec3> Normals(NumVertex);
    std::vector<int> Counts(NumVertex);
    for (size_t i = 0; i < NumVertex; ++i)
    {
        Normals[i] = glm::vec3(0.0, 0.0, 0.0);
        Counts[i] = 0;
    }
    for (size_t i = 0; i < m_pObj->Faces.size(); ++i)
    {
        glm::vec3 V1 = getVertex(i, 1) - getVertex(i, 0);
        glm::vec3 V2 = getVertex(i, 2) - getVertex(i, 0);
        glm::vec3 Normal = glm::normalize(glm::cross(V1, V2));
        for (size_t k = 0; k < getFaceNodeNum(i); k++)
        {
            uint32_t VertexIndex = m_pObj->Faces[i].Nodes[k].VectexId - 1;
            Normals[VertexIndex] += Normal;
            Counts[VertexIndex]++;
        }
    }
    for (size_t i = 0; i < NumVertex; ++i)
        Normals[i] /= static_cast<double>(Counts[i]);
    return Normals;
}

std::vector<glm::vec2> CIOObj::getRandomTexCoordPerVertex()
{
    size_t NumVertex = m_pObj->Vertices.size();

    std::vector<glm::vec2> TexCoords(NumVertex);
    std::default_random_engine RandomEngine;
    RandomEngine
    for (size_t i = 0; i < NumVertex; ++i)
    {
        double RandomU = static_cast<double>(RandomEngine()) / (RandomEngine.max() - RandomEngine.min());
        double RandomV = static_cast<double>(RandomEngine()) / (RandomEngine.max() - RandomEngine.min());
        TexCoords[i] = glm::vec2(RandomU, RandomV);
    }
    return TexCoords;
}