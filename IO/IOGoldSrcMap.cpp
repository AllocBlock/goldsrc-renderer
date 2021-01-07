#include "IOGoldSrcMap.h"

#include <fstream>
#include <regex>
#include <sstream>

float CMapBrush::GlobalScale = 1.0 / 64.0;

// vertice are stored at clockwise
glm::vec3 CMapPlane::getNormal() const
{
    return correctNormal(glm::normalize(glm::cross(Points[2] - Points[0], Points[1] - Points[0])));
}

float CMapPlane::getDistanceToFace(glm::vec3 vPoint) const
{
    return glm::dot(vPoint - Points[0], getNormal());
}

glm::vec3 CMapPlane::correctNormal(glm::vec3 vN) const
{
    // solve accuracy problem which causes a negative zero
    float Epsilon = 0.05;
    if (vN.x > -Epsilon && vN.x < Epsilon) vN.x = 0;
    if (vN.y > -Epsilon && vN.y < Epsilon) vN.y = 0;
    if (vN.z > -Epsilon && vN.z < Epsilon) vN.z = 0;
    return vN;
}

std::vector<glm::vec2> CMapPolygon::getTexCoords(size_t vTexWidth, size_t vTexHeight)
{
    if (!m_TexCoords.empty()) return m_TexCoords;

    for (const glm::vec3& Vertex : Vertices)
        m_TexCoords.emplace_back(__calcTexCoord(Vertex, vTexWidth, vTexHeight));
    
    return m_TexCoords;
}

glm::vec2 CMapPolygon::__calcTexCoord(glm::vec3 vVertex, size_t vTexWidth, size_t vTexHeight)
{
    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex / CMapBrush::GlobalScale, pPlane->TextureDirectionU) / pPlane->TextureScaleU + pPlane->TextureOffsetU) / vTexWidth;
    TexCoord.y = (glm::dot(vVertex / CMapBrush::GlobalScale, pPlane->TextureDirectionV) / pPlane->TextureScaleV + pPlane->TextureOffsetV) / vTexHeight;
    return TexCoord;
}

std::vector<CMapPolygon> CMapBrush::getPolygons()
{
    if (!m_Polygons.empty()) return m_Polygons;
    for (size_t i = 0; i < Planes.size(); ++i)
    {
        CMapPolygon Polygon;
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
                    IntersectionPoint *= CMapBrush::GlobalScale;
                    m_Polygons[i1].Vertices.emplace_back(IntersectionPoint);
                    m_Polygons[i2].Vertices.emplace_back(IntersectionPoint);
                    m_Polygons[i3].Vertices.emplace_back(IntersectionPoint);
                }
                else
                {
                    // TODO:: judge if two plane is the same, which cause illegal brush
                    // GlobalLogger::logStream() << "some planes in brush " << this << "are the same" << std::endl;
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

bool CMapBrush::__getIntersection(glm::vec3& voPoint, size_t vPlane1, size_t vPlane2, size_t vPlane3)
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
    for (CMapPlane pPlane : Planes)
    {
        float D = pPlane.getDistanceToFace(IntersectionPoint);
        if (D > Epsilon)
            return false;
    }

    voPoint = IntersectionPoint;
    return true;
}

void CMapBrush::sortVerticesInClockwise(std::vector<glm::vec3>& vVertices, const glm::vec3 vNormal)
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

bool CIOGoldSrcMap::_readV(std::string vFileName)
{
    std::ifstream File;
    File.open(vFileName, std::ios::in);
    if (!File.is_open())
    {
        GlobalLogger::logStream() << "read file " << vFileName << " failed" << std::endl;
        return false;
    }

    std::string Text((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());

    uint32_t EntityStartIndex = 0;
    int PairingLevel = 0;
    for (size_t i = 0; i < Text.length(); ++i)
    {
        if (Text[i] == '{')
        {
            if (PairingLevel == 0)
            {
                EntityStartIndex = i;
            }
            if (PairingLevel == 2) continue; // max level is 2, and need to void { appears in texture name
            PairingLevel++;
        }
        else if (Text[i] == '}')
        {
            PairingLevel--;
            if (PairingLevel == 0)
            {
                std::string EntityText = Text.substr(EntityStartIndex, i - EntityStartIndex + 1);
                CMapEntity Entity;
                Entity.read(EntityText);
                m_Entities.push_back(Entity);
            }
        }
        else if (PairingLevel == 0 && !isWhiteSpace(Text[i]))
        {
            return false;
        }
    }
    if (PairingLevel > 0) return false;

    return true;
}

void CMapEntity::read(std::string vTextEntity)
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

void CMapEntity::__readProperties(std::string vTextProperties)
{
    vTextProperties = CIOBase::trimString(vTextProperties);

    std::vector<std::string> TextProperties = CIOBase::splitString(vTextProperties, '\n');
    for (const std::string& TextProperty : TextProperties)
        __readProperty(TextProperty);
}

void CMapEntity::__readProperty(std::string vTextProperty)
{
    vTextProperty = CIOBase::trimString(vTextProperty);
    std::smatch Results;

    static const std::regex ReKeyValue("\"(.*?)\"\\s+\"(.*?)\"");

    if (!std::regex_match(vTextProperty, Results, ReKeyValue))
        throw "map file parse failed";
    std::string Key = Results[1].str();
    std::string Value = Results[2].str();
    Properties[Key] = Value;
}

void CMapEntity::__readBrushes(std::string vTextBrushes)
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
            if (!InBrush) throw "map file parse failed";
            std::string TextBrush = vTextBrushes.substr(StartBrushIndex, i - StartBrushIndex);
            __readBrush(TextBrush);
            InBrush = false;
        }
        else if (!InBrush && !CIOBase::isWhiteSpace(vTextBrushes[i]))
        {
            throw "map file parse failed";
        }
    }
    if (InBrush) throw "map file parse failed";
}

void CMapEntity::__readBrush(std::string vTextBrush)
{
    vTextBrush = CIOBase::trimString(vTextBrush);

    std::vector<std::string> TextPlanes = CIOBase::splitString(vTextBrush, '\n');

    CMapBrush Brush;
    for (const std::string& TextPlane : TextPlanes)
    {
        CMapPlane pPlane = __parsePlane(TextPlane);
        Brush.Planes.emplace_back(pPlane);
    }
    Brushes.emplace_back(Brush);
}

CMapPlane CMapEntity::__parsePlane(std::string vTextPlane)
{
    vTextPlane = CIOBase::trimString(vTextPlane);
    std::smatch Results;

    static const std::regex RePlaneInfo("\\((.*?)\\)\\s*\\((.*?)\\)\\s*\\((.*?)\\)\\s*(.*?)\\s*\\[(.*?)\\]\\s*\\[(.*?)\\]\\s*(\\S*?)\\s*(\\S*?)\\s*(\\S*?)");
    if (!std::regex_match(vTextPlane, Results, RePlaneInfo))
        throw "map file parse failed";

    CMapPlane pPlane = {};
    for (int i = 0; i < 3; ++i)
    {
        const std::vector<float> Vertex = __parseFloatArray(Results[i+1]);
        _ASSERTE(Vertex.size() == 3);
        pPlane.Points[i] = glm::vec3(Vertex[0], Vertex[1], Vertex[2]);
    }
    pPlane.TextureName = Results[4];
    const std::vector<float> UInfo = __parseFloatArray(Results[5]);
    _ASSERTE(UInfo.size() == 4);
    pPlane.TextureDirectionU = glm::vec3(UInfo[0], UInfo[1], UInfo[2]);
    pPlane.TextureOffsetU = UInfo[3];
    const std::vector<float> VInfo = __parseFloatArray(Results[6]);
    _ASSERTE(VInfo.size() == 4);
    pPlane.TextureDirectionV = glm::vec3(VInfo[0], VInfo[1], VInfo[2]);
    pPlane.TextureOffsetV = VInfo[3];
    pPlane.TextureRotation = __parseFloat(Results[7]);
    pPlane.TextureScaleU = __parseFloat(Results[8]);
    pPlane.TextureScaleV = __parseFloat(Results[9]);

    return pPlane;
}

std::vector<float> CMapEntity::__parseFloatArray(std::string vText)
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

float CMapEntity::__parseFloat(std::string vText)
{
    return atof(vText.c_str());
}

std::vector<std::string> CIOGoldSrcMap::getWadPaths()
{
    std::vector<std::string> WadPaths;

    for (int i = 0; i < m_Entities.size(); ++i)
    {
        auto Properties = m_Entities[i].Properties;
        if (Properties.find("classname") != Properties.end() && Properties["classname"] == "worldspawn")
        {
            if (Properties.find("wad") == Properties.end())
            {
                throw "wad property not found in worldspawn entity";
            }
            else
            {
                return CIOBase::splitString(Properties["wad"], ';');
            }
        }
    }
    throw "worldspawn entity not found";
}

std::set<std::string> CIOGoldSrcMap::getUsedTextureNames()
{
    std::set<std::string> UsedTextureNames;
    for (const CMapEntity& Entity : m_Entities)
    {
        for (const CMapBrush& Brush : Entity.Brushes)
        {
            for (const CMapPlane& pPlane : Brush.Planes)
            {
                UsedTextureNames.insert(pPlane.TextureName);
            }
        }
    }
    return UsedTextureNames;
}

std::vector<CMapPolygon> CIOGoldSrcMap::getAllPolygons()
{
    std::vector<CMapPolygon> Polygons;
    for (CMapEntity& Entity : m_Entities)
    {
        for (CMapBrush& Brush : Entity.Brushes)
        {
            std::vector<CMapPolygon> BrushPolygons = Brush.getPolygons();
            Polygons.insert(Polygons.end(), BrushPolygons.begin(), BrushPolygons.end());
        }
    }
    return Polygons;
}

std::string CIOGoldSrcMap::toString()
{
    std::ostringstream StringStream;
    StringStream << m_FileName << " map file info:" << std::endl;
    StringStream << "Entitiy Count: " << m_Entities.size() << std::endl;
    StringStream << "Used texture names: " << std::endl;
    auto TexNames = getUsedTextureNames();
    for (std::string TexName : TexNames)
        StringStream << "\t" << TexName << std::endl;
    return StringStream.str();
}
