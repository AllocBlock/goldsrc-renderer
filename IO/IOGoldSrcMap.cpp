#include "IOGoldSrcMap.h"

#include <fstream>
#include <regex>
#include <sstream>

float SMapBrush::GlobalScale = 1.0f / 64.0f;
const std::string g_ParseFailMessage = u8"MAP文件解析失败";

// vertice are stored at clockwise
glm::vec3 SMapPlane::getNormal() const
{
    return correctNormal(glm::normalize(glm::cross(Points[2] - Points[0], Points[1] - Points[0])));
}

float SMapPlane::getDistanceToFace(glm::vec3 vPoint) const
{
    return glm::dot(vPoint - Points[0], getNormal());
}

glm::vec3 SMapPlane::correctNormal(glm::vec3 vN) const
{
    // solve accuracy problem which causes a negative zero
    float Epsilon = 0.05;
    if (vN.x > -Epsilon && vN.x < Epsilon) vN.x = 0;
    if (vN.y > -Epsilon && vN.y < Epsilon) vN.y = 0;
    if (vN.z > -Epsilon && vN.z < Epsilon) vN.z = 0;
    return vN;
}

std::vector<glm::vec2> SMapPolygon::getTexCoords(size_t vTexWidth, size_t vTexHeight)
{
    if (!m_TexCoords.empty()) return m_TexCoords;

    for (const glm::vec3& Vertex : Vertices)
        m_TexCoords.emplace_back(__calcTexCoord(Vertex, vTexWidth, vTexHeight));
    
    return m_TexCoords;
}

glm::vec2 SMapPolygon::__calcTexCoord(glm::vec3 vVertex, size_t vTexWidth, size_t vTexHeight)
{
    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex / SMapBrush::GlobalScale, pPlane->TextureDirectionU) / pPlane->TextureScaleU + pPlane->TextureOffsetU) / vTexWidth;
    TexCoord.y = (glm::dot(vVertex / SMapBrush::GlobalScale, pPlane->TextureDirectionV) / pPlane->TextureScaleV + pPlane->TextureOffsetV) / vTexHeight;
    return TexCoord;
}

std::vector<SMapPolygon> SMapBrush::getPolygons()
{
    if (!m_Polygons.empty()) return m_Polygons;
    for (size_t i = 0; i < Planes.size(); ++i)
    {
        SMapPolygon Polygon;
        Polygon.pPlane = &Planes[i];
        m_Polygons.emplace_back(Polygon);
    }

    // iterate every 3 planes and get intersection
    for (size_t i1 = 0; i1 < Planes.size() - 2; ++i1)
    {
        for (size_t i2 = i1 + 1; i2 < Planes.size() - 1; ++i2)
        {
            for (size_t i3 = i2 + 1; i3 < Planes.size(); ++i3)
            {
                glm::vec3 IntersectionPoint;
                if (__getIntersection(IntersectionPoint, i1, i2, i3))
                {
                    IntersectionPoint *= SMapBrush::GlobalScale;
                    m_Polygons[i1].Vertices.emplace_back(IntersectionPoint);
                    m_Polygons[i2].Vertices.emplace_back(IntersectionPoint);
                    m_Polygons[i3].Vertices.emplace_back(IntersectionPoint);
                }
                else
                {
                    globalLog(u8"map文件中存在疑似非法固体（同一个固体中存在相同的平面），可能导致渲染效果出错");
                }
            }
        }
    }

    for (size_t i = 0; i < m_Polygons.size(); ++i)
    {
        sortVerticesInClockwise(m_Polygons[i].Vertices, m_Polygons[i].pPlane->getNormal());
    }

    return m_Polygons;
}

bool SMapBrush::__getIntersection(glm::vec3& voPoint, size_t vPlane1, size_t vPlane2, size_t vPlane3)
{
    glm::vec3 N1 = Planes[vPlane1].getNormal();
    glm::vec3 N2 = Planes[vPlane2].getNormal();
    glm::vec3 N3 = Planes[vPlane3].getNormal();
    float D1 = Planes[vPlane1].getDistanceToFace();
    float D2 = Planes[vPlane2].getDistanceToFace();
    float D3 = Planes[vPlane3].getDistanceToFace();

    float MixeProduct = glm::dot(N1, glm::cross(N2, N3));

    if (MixeProduct == 0) return false;

    // Cramer's Rule to solve the intersection
    glm::vec3 IntersectionPoint = (-D1 * cross(N2, N3) + -D2 * cross(N3, N1) + -D3 * cross(N1, N2)) / MixeProduct;

    // the intersection might be not in the brush (see the map file spec. for example)
    // we need to check if it is using the property of convex polyhedron: 
    // if a point is on the brush, this point must be on or behind each plane (normal direction is front) 
    const float Epsilon = 1e-3;
    for (SMapPlane pPlane : Planes)
    {
        float D = pPlane.getDistanceToFace(IntersectionPoint);
        if (D > Epsilon)
            return false;
    }

    voPoint = IntersectionPoint;
    return true;
}

void SMapBrush::sortVerticesInClockwise(std::vector<glm::vec3>& vVertices, const glm::vec3 vNormal)
{
    if (vVertices.size() < 3) return;

    // get polygon center
    glm::vec3 Center = { 0.0f, 0.0f, 0.0f };
    for (const glm::vec3& Vertex : vVertices)
        Center += Vertex;
    Center /= vVertices.size();

    // simple selection sort
    for (size_t i = 0; i < vVertices.size() - 2; ++i)
    {
        glm::vec3 BaseRadialDirection = glm::normalize(vVertices[i] - Center);
        double MaxCos = -1; // minest angle
        int NextVertexIndex = -1;
        glm::vec3 ScanDirection = -glm::normalize(glm::cross(vNormal, BaseRadialDirection)); // clockwise

        for (int k = i + 1; k < vVertices.size(); ++k)
        {
            glm::vec3 CurRadialDirection = glm::normalize(vVertices[k] - Center);
            if (glm::dot(ScanDirection, CurRadialDirection) >= 0) 
            {
                double Cos = glm::dot(BaseRadialDirection, CurRadialDirection);
                if (Cos >= MaxCos)
                {
                    MaxCos = Cos;
                    NextVertexIndex = k;
                }
            }
        }
        glm::vec3 Temp = vVertices[i + 1];
        vVertices[i + 1] = vVertices[NextVertexIndex];
        vVertices[NextVertexIndex] = Temp;
    }
}

bool CIOGoldSrcMap::readFromString(std::string vText)
{
    uint32_t EntityStartIndex = 0;
    int PairingLevel = 0;
    for (size_t i = 0; i < vText.length(); ++i)
    {
        if (vText[i] == '{')
        {
            if (PairingLevel == 0)
            {
                EntityStartIndex = i;
            }
            if (PairingLevel == 2) continue; // max level is 2, and need to void { appears in texture name
            PairingLevel++;
        }
        else if (vText[i] == '}')
        {
            PairingLevel--;
            if (PairingLevel == 0)
            {
                std::string EntityText = vText.substr(EntityStartIndex, i - EntityStartIndex + 1);
                SMapEntity Entity;
                Entity.read(EntityText);
                m_Entities.push_back(Entity);
            }
        }
        else if (PairingLevel == 0 && !isWhiteSpace(vText[i]))
        {
            return false;
        }
    }
    if (PairingLevel > 0) return false;

    return true;
}

bool CIOGoldSrcMap::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File;
    File.open(vFilePath, std::ios::in);
    if (!File.is_open())
    {
        globalLog(u8"打开文件 [" + vFilePath.u8string() + u8"] 失败，无权限或文件不存在");
        return false;
    }

    std::string Text((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
    return readFromString(Text);
}

void SMapEntity::read(std::string vTextEntity)
{
    vTextEntity = CIOBase::trimString(vTextEntity);

    size_t LastQuote = 1;
    for (size_t i = 1; i < vTextEntity.length() - 1; ++i)
    {
        if (vTextEntity[i] == '"') LastQuote = i;
        else if (vTextEntity[i] == '{') break;
    }

    std::string TextProperties = vTextEntity.substr(1, LastQuote);
    std::string TextBrush = vTextEntity.substr(LastQuote + 1, vTextEntity.length() - LastQuote - 2);

    __readProperties(TextProperties);
    __readBrushes(TextBrush);
}

void SMapEntity::__readProperties(std::string vTextProperties)
{
    vTextProperties = CIOBase::trimString(vTextProperties);

    std::vector<std::string> TextProperties = CIOBase::splitString(vTextProperties, '\n');
    for (const std::string& TextProperty : TextProperties)
        __readProperty(TextProperty);
}

void SMapEntity::__readProperty(std::string vTextProperty)
{
    vTextProperty = CIOBase::trimString(vTextProperty);
    std::smatch Results;

    static const std::regex ReKeyValue("\"(.*?)\"\\s+\"(.*?)\"");

    if (!std::regex_match(vTextProperty, Results, ReKeyValue))
        throw std::runtime_error(g_ParseFailMessage);
    std::string Key = Results[1].str();
    std::string Value = Results[2].str();
    Properties[Key] = Value;
}

void SMapEntity::__readBrushes(std::string vTextBrushes)
{
    vTextBrushes = CIOBase::trimString(vTextBrushes);

    size_t StartBrushIndex = 0;
    bool InBrush = false;
    for (size_t i = 0; i < vTextBrushes.length(); ++i)
    {
        if (vTextBrushes[i] == '{' && !InBrush)
        {
            InBrush = true;
            StartBrushIndex = i + 1;
        }
        else if (vTextBrushes[i] == '}')
        {
            if (!InBrush) throw std::runtime_error(g_ParseFailMessage);
            std::string TextBrush = vTextBrushes.substr(StartBrushIndex, i - StartBrushIndex);
            __readBrush(TextBrush);
            InBrush = false;
        }
        else if (!InBrush && !CIOBase::isWhiteSpace(vTextBrushes[i]))
        {
            throw std::runtime_error(g_ParseFailMessage);
        }
    }
    if (InBrush) throw std::runtime_error(g_ParseFailMessage);
}

void SMapEntity::__readBrush(std::string vTextBrush)
{
    vTextBrush = CIOBase::trimString(vTextBrush);

    std::vector<std::string> TextPlanes = CIOBase::splitString(vTextBrush, '\n');

    SMapBrush Brush;
    for (const std::string& TextPlane : TextPlanes)
    {
        SMapPlane pPlane = __parsePlane(TextPlane);
        Brush.Planes.emplace_back(pPlane);
    }
    Brushes.emplace_back(Brush);
}

SMapPlane SMapEntity::__parsePlane(std::string vTextPlane)
{
    vTextPlane = CIOBase::trimString(vTextPlane);
    // (x1 y1 z1) (x2 y2 z2) (x3 y3 z3) TexName [ ux uy uz uOffset ] [ vx vy vz vOffset ] rotate uScale vScale
    SMapPlane Plane = {};
    std::stringstream PlaneSStream(vTextPlane);

    for (int i = 0; i < 3; ++i)
    {
        while (CIOBase::isWhiteSpace(PlaneSStream.peek())) PlaneSStream.get();
        if (PlaneSStream.get() != '(') throw std::runtime_error(g_ParseFailMessage);
        PlaneSStream >> Plane.Points[i].x;
        PlaneSStream >> Plane.Points[i].y;
        PlaneSStream >> Plane.Points[i].z;
        while (CIOBase::isWhiteSpace(PlaneSStream.peek())) PlaneSStream.get();
        if (PlaneSStream.get() != ')') throw std::runtime_error(g_ParseFailMessage);
    }
    PlaneSStream >> Plane.TextureName;

    while (CIOBase::isWhiteSpace(PlaneSStream.peek())) PlaneSStream.get();
    if (PlaneSStream.get() != '[') throw std::runtime_error(g_ParseFailMessage);
    PlaneSStream >> Plane.TextureDirectionU.x;
    PlaneSStream >> Plane.TextureDirectionU.y;
    PlaneSStream >> Plane.TextureDirectionU.z;
    PlaneSStream >> Plane.TextureOffsetU;
    while (CIOBase::isWhiteSpace(PlaneSStream.peek())) PlaneSStream.get();
    if (PlaneSStream.get() != ']') throw std::runtime_error(g_ParseFailMessage);

    while (CIOBase::isWhiteSpace(PlaneSStream.peek())) PlaneSStream.get();
    if (PlaneSStream.get() != '[') throw std::runtime_error(g_ParseFailMessage);
    PlaneSStream >> Plane.TextureDirectionV.x;
    PlaneSStream >> Plane.TextureDirectionV.y;
    PlaneSStream >> Plane.TextureDirectionV.z;
    PlaneSStream >> Plane.TextureOffsetV;
    while (CIOBase::isWhiteSpace(PlaneSStream.peek())) PlaneSStream.get();
    if (PlaneSStream.get() != ']') throw std::runtime_error(g_ParseFailMessage);

    PlaneSStream >> Plane.TextureRotation;
    PlaneSStream >> Plane.TextureScaleU;
    PlaneSStream >> Plane.TextureScaleV;

    return Plane;
}

std::vector<float> SMapEntity::__parseFloatArray(std::string vText)
{
    vText = CIOBase::trimString(vText);
    std::vector<std::string> TextNums = CIOBase::splitString(vText);

    std::vector<float> FloatArray;
    for (const std::string& TextNum : TextNums)
    {
        float Num = __parseFloat(TextNum);
        FloatArray.emplace_back(Num);
    }
    return FloatArray;
}

float SMapEntity::__parseFloat(std::string vText)
{
    return atof(vText.c_str());
}

std::vector<std::filesystem::path> CIOGoldSrcMap::getWadPaths()
{
    for (int i = 0; i < m_Entities.size(); ++i)
    {
        auto Properties = m_Entities[i].Properties;
        if (Properties.find("classname") != Properties.end() && Properties["classname"] == "worldspawn")
        {
            if (Properties.find("wad") == Properties.end())
            {
                globalLog(u8"未在worldspawn实体内找到wad文件信息");
                return {};
            }
            else
            {
                std::vector<std::string> WadNames = CIOBase::splitString(Properties["wad"], ';');
                std::vector<std::filesystem::path> WadPaths(WadNames.begin(), WadNames.end());
                return WadPaths;
            }
        }
    }
    throw std::runtime_error(u8"未找到worldspawn实体，无法找到wad文件信息");
}

std::string CIOGoldSrcMap::getSkyBoxPrefix()
{
    for (int i = 0; i < m_Entities.size(); ++i)
    {
        auto Properties = m_Entities[i].Properties;
        if (Properties.find("classname") != Properties.end() && Properties["classname"] == "worldspawn")
        {
            if (Properties.find("skyname") == Properties.end())
            {
                globalLog(u8"未在worldspawn实体内找到wad文件信息");
                return "";
            }
            else
            {
                return Properties["skyname"];
            }
        }
    }
    throw std::runtime_error(u8"未找到worldspawn实体，无法找到wad文件信息");
}

std::set<std::string> CIOGoldSrcMap::getUsedTextureNames()
{
    std::set<std::string> UsedTextureNames;
    for (const SMapEntity& Entity : m_Entities)
    {
        for (const SMapBrush& Brush : Entity.Brushes)
        {
            for (const SMapPlane& pPlane : Brush.Planes)
            {
                UsedTextureNames.insert(pPlane.TextureName);
            }
        }
    }
    return UsedTextureNames;
}

std::vector<SMapPolygon> CIOGoldSrcMap::getAllPolygons()
{
    std::vector<SMapPolygon> Polygons;
    for (SMapEntity& Entity : m_Entities)
    {
        for (SMapBrush& Brush : Entity.Brushes)
        {
            std::vector<SMapPolygon> BrushPolygons = Brush.getPolygons();
            Polygons.insert(Polygons.end(), BrushPolygons.begin(), BrushPolygons.end());
        }
    }
    return Polygons;
}

std::string CIOGoldSrcMap::toString()
{
    std::ostringstream StringStream;
    StringStream << m_FilePath << " map file info:" << std::endl;
    StringStream << "Entitiy Count: " << m_Entities.size() << std::endl;
    StringStream << "Used texture names: " << std::endl;
    auto TexNames = getUsedTextureNames();
    for (std::string TexName : TexNames)
        StringStream << "\t" << TexName << std::endl;
    return StringStream.str();
}
