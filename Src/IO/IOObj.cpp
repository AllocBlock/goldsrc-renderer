#include "IOObj.h"
#include "Log.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <random>

bool CIOObj::_readV(std::filesystem::path vFilePath) {
    m_pObj = make<SObj>();

    if (!vFilePath.empty()) m_FilePath = vFilePath;
    _ASSERTE(std::filesystem::exists(m_FilePath));

    std::ifstream File;
    File.open(m_FilePath);
    if (!File.is_open()) {
        Log::log(vFilePath.u8string() + " 文件打开失败");
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
                    Log::log(" 面数据错误" + std::string(LineBuffer));
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
                m_pMtl = make<CIOMtl>();
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
            Log::log(m_FilePath.u8string() + " obj格式有误或不支持的标记：" + Cmd);
            continue;
        }
    }

    return true;
}

size_t CIOObj::getFaceNum() const
{ 
    if (m_pObj) return m_pObj->Faces.size();
    else return 0;
}

size_t CIOObj::getFaceNodeNum(size_t vFaceIndex) const
{
    if (!m_pObj || vFaceIndex < 0 || vFaceIndex >= getFaceNum()) return 0;
    return m_pObj->Faces[vFaceIndex].Nodes.size();
}

bool CIOObj::dumpVertex(size_t vFaceIndex, size_t vNodeIndex, glm::vec3& voVertex) const
{
    if (!m_pObj || 
        vFaceIndex < 0 || vFaceIndex >= getFaceNum() ||
        vNodeIndex < 0 || vNodeIndex >= m_pObj->Faces[vFaceIndex].Nodes.size())
        return false;
    uint32_t VertexId = m_pObj->Faces[vFaceIndex].Nodes[vNodeIndex].VectexId;
    voVertex = m_pObj->Vertices[VertexId - 1];
    return true;
}

bool CIOObj::dumpTexCoord(size_t vFaceIndex, size_t vNodeIndex, glm::vec2& voTexCoord) const
{
    if (!m_pObj ||
        vFaceIndex < 0 || vFaceIndex >= getFaceNum() ||
        vNodeIndex < 0 || vNodeIndex >= m_pObj->Faces[vFaceIndex].Nodes.size())
        return false;
    uint32_t TexCoordId = m_pObj->Faces[vFaceIndex].Nodes[vNodeIndex].TexCoordId;
    if (TexCoordId == 0)
        return false;
    // 纹理映射方式不同，obj->vulkan
    voTexCoord = m_pObj->TexCoords[TexCoordId - 1];
    voTexCoord.y = 1.0f - voTexCoord.y;
    return true;
}

bool CIOObj::dumpNormal(size_t vFaceIndex, size_t vNodeIndex, glm::vec3& voNormal) const
{
    if (!m_pObj ||
        vFaceIndex < 0 || vFaceIndex >= getFaceNum() ||
        vNodeIndex < 0 || vNodeIndex >= m_pObj->Faces[vFaceIndex].Nodes.size())
        return false;
    uint32_t NormalId = m_pObj->Faces[vFaceIndex].Nodes[vNodeIndex].NormalId;
    if (NormalId == 0)
        return false;
    voNormal = m_pObj->Normals[NormalId - 1];
    return true;
}