#include "IOGoldSrcMap.h"

#include <fstream>
#include <regex>

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
    for(size_t i = 0; i < Text.length(); i++)
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

// vertice are stored at clockwise
glm::vec3 CMapPlane::getNormal()
{
    return correctNormal(normalize(cross(Points[2] - Points[0], Points[1] - Points[0])));
}

float CMapPlane::getDistanceToFace(glm::vec3 vPoint = glm::vec3(0, 0, 0))
{
    return dot(Points[0] - vPoint, getNormal());
}

glm::vec3 CMapPlane::correctNormal(glm::vec3 vN)
{
    // solve accuracy problem which causes a negative zero
    float Epsilon = 0.05;
    if (vN.x > -Epsilon && vN.x < Epsilon) vN.x = 0;
    if (vN.y > -Epsilon && vN.y < Epsilon) vN.y = 0;
    if (vN.z > -Epsilon && vN.z < Epsilon) vN.z = 0;
    return vN;
}

//std::vector<MapPolygon> CMapBrush::GetPolygonList()
//{
//    vector<MapPolygon> polygonList;
//    polygonList.resize(Planes.size());
//
//    // 填写纹理信息
//
//    for (int i = 0; i < (int)Planes.size(); i++) {
//        polygonList[i].plane = &Planes[i];
//    }
//
//    // 三个面一组遍历
//    for (int i = 0; i < (int)Planes.size() - 2; i++) {
//        for (int j = i + 1; j < (int)Planes.size() - 1; j++) {
//            for (int k = j + 1; k < Planes.size(); k++) {
//                vec3 iPoint;
//                if (GetIntersection(iPoint, i, j, k)) {
//                    polygonList[i].vertexList.push_back(iPoint);
//                    polygonList[j].vertexList.push_back(iPoint);
//                    polygonList[k].vertexList.push_back(iPoint);
//                }
//            }
//        }
//    }
//    SortVertices(polygonList);
//    return polygonList;
//}
//
//bool CMapBrush::GetIntersection(vec3& iPoint, int plane1, int plane2, int plane3)
//{
//
//    // if (i == j || i == k) continue;
//    vec3 n1 = Planes[plane1].getNormal();
//    vec3 n2 = Planes[plane2].getNormal();
//    vec3 n3 = Planes[plane3].getNormal();
//    float d1 = Planes[plane1].getDistanceToFace();
//    float d2 = Planes[plane2].getDistanceToFace();
//    float d3 = Planes[plane3].getDistanceToFace();
//
//    /*
//    // debug输出
//    cout << "-------------计算面交点--------------" << endl;
//    cout << i << ": " << n1.x << ", " << n1.y << ", " << n1.z << ", " << endl;
//    cout << j << ": " << n2.x << ", " << n2.y << ", " << n2.z << ", " << endl;
//    cout << k << ": " << n3.x << ", " << n3.y << ", " << n3.z << ", " << endl;
//    cout << "denom: " << dot(n1, cross(n2, n3)) << endl;
//    cout << "-------------------------------------" << endl << endl;
//    */
//
//
//    float denom = dot(n1, cross(n2, n3));            // 若denominator为0，说明三平面没有交点、或有无限个交点，否则应该只有一个交点
//    // denominator相当于三阶行列式
//
//    if (denom == 0) return false;
//
//    // 要得出三面交点，相当于一个齐次方程，这里用克拉默法则
//    //     | d1 n1y n1z |                            | 1   1   1  |
//    // x = | d2 n2y n2z |  / denom; cross(n2, n3) =  | d2 n2y n2z |
//    //     | d3 n3y n3z |                             | d3 n3y n3z |
//    vec3 v = (d1 * cross(n2, n3) + d2 * cross(n3, n1) + d3 * cross(n1, n2)) / denom;
//    // 若denominator不为0，不能说明就有交点，因为还要考虑平面位置（我们之前只计算法向量，即默认平面经过原点
//    // 因为所有固体都是凸多面体，可知如果一个顶点属于这个固体，则这个顶点一定所有模型面的后方（法线方向为前方）
//    //cout << "判断顶点" << v.x << ", " << v.y << ", " << v.z << ", " << endl;
//    bool illegal = false;
//    int t = 0;
//    const float epsilon = 0.05;
//    for (CMapPlane plane : Planes) {
//        if (dot(plane.getNormal(), v) > (plane.getDistanceToFace() + epsilon)) {
//
//            //if (dot(plane.GetNormal(), v - plane.pointList[0]) > 0) {
//            illegal = true;
//            break;
//        }
//        //cout << "在" << t++ << "后侧里" << endl;
//
//    }
//    if (illegal) return false;
//
//    iPoint = v;
//    return true;
//
//}
//
//void CMapBrush::SortVertices(vector<MapPolygon>& polygonList)
//{
//    for (MapPolygon& polygon : polygonList) {
//        if (polygon.vertexList.size() < 3) continue;
//
//        // 计算中心
//        vec3 center = { 0.0f, 0.0f, 0.0f };
//        for (vec3& v : polygon.vertexList) {
//            center += v;
//        }
//        center /= polygon.vertexList.size();
//
//        vec3 normal = polygon.plane->getNormal();
//        // 选择排序
//        for (int i = 0; i < (int)polygon.vertexList.size() - 2; i++) {
//            vec3 a = normalize(polygon.vertexList[i] - center);
//            double smallestAngle = -1;
//            int smallest = -1;
//            vec3 direct = cross(normal, a);
//
//            for (int j = i + 1; j < polygon.vertexList.size(); j++) {
//                vec3 b = normalize(polygon.vertexList[j] - center);
//                if (dot(direct, b) <= 0) {            // >=是逆时针
//                    double angle = dot(a, b);
//                    if (angle > smallestAngle)
//                    {
//                        smallestAngle = angle;
//                        smallest = j;
//                    }
//                }
//            }
//            vec3 temp = polygon.vertexList[i + 1];
//            polygon.vertexList[i + 1] = polygon.vertexList[smallest];
//            polygon.vertexList[smallest] = temp;
//        }
//    }
//}
//
//vector<MapPolygon> CMapEntity::GetPolygonList() {
//    vector<MapPolygon> res;
//    for (CMapBrush brush : Brushes) {
//        vector<MapPolygon> brushPolygonList = brush.GetPolygonList();
//        res.insert(res.end(), brushPolygonList.begin(), brushPolygonList.end());
//    }
//    return res;
//}

std::vector<std::string> CIOGoldSrcMap::getWadPaths()
{
    std::vector<std::string> WadPaths;

    for (int i = 0; i < m_Entities.size(); i++)
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

//vector<MapPolygon> CIOGoldSrcMap::GetPolygonList() {
//    cout << "计算顶点和顶点排序" << endl;
//    vector<MapPolygon> res;
//    for (CMapEntity entity : m_Entities) {
//        vector<MapPolygon> entityPolygonList = entity.GetPolygonList();
//        res.insert(res.end(), entityPolygonList.begin(), entityPolygonList.end());
//    }
//    return res;
//}
//
//vector<string> CIOGoldSrcMap::GetUsedTextureList() {
//    cout << "搜寻已使用的纹理" << endl;
//    vector<string> usedTexture;
//    for (CMapEntity entity : m_Entities) {
//        for (CMapBrush brush : entity.Brushes) {
//            for (CMapPlane polygon : brush.Planes) {
//                bool alreadyHas = false;
//                for (string name : usedTexture) {
//                    if (name == polygon.TextureName) {
//                        alreadyHas = true;
//                        break;
//                    }
//                }
//                if (!alreadyHas) {
//                    usedTexture.push_back(polygon.TextureName);
//                    //cout << polygon.textureName << endl;
//                }
//            }
//        }
//    }
//    // 全部转为大写
//    for (string& name : usedTexture) {
//        name = ToUpperCase(name);
//    }
//    return usedTexture;
//}

//void CIOGoldSrcMap::PrintMapInfo() {
//    cout << "地图共含有" << m_Entities.size() << "个物体" << endl;
//    cout << "使用的wad：" << endl;
//    for (string wadName : GetWadList()) {
//        cout << "\t" << wadName << endl;
//    }
//    cout << "使用的纹理：" << endl;
//    for (string texName : GetUsedTextureList()) {
//        cout << "\t" << texName << endl;
//    }
//}
//
//vec2 MapParser::getTexCoordinates(MapPolygon polygon, int textureWidth, int textureHeight, int index) {
//    vec2 res;
//    res.x = (dot(polygon.vertexList[index], polygon.plane->TextureDirectionU) / polygon.plane->TextureScaleU + polygon.plane->TextureOffsetU) / textureWidth;
//    res.y = (dot(polygon.vertexList[index], polygon.plane->TextureDirectionV) / polygon.plane->TextureScaleV + polygon.plane->TextureOffsetV) / textureHeight;
//    return res;
//}

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
        CMapPlane Plane = __parsePlane(TextPlane);
        Brush.Planes.emplace_back(Plane);
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

    CMapPlane Plane = {};
    for (int i = 0; i < 3; ++i)
    {
        const std::vector<float> Vertex = __parseFloatArray(Results[i+1]);
        _ASSERTE(Vertex.size() == 3);
        Plane.Points[i] = glm::vec3(Vertex[0], Vertex[1], Vertex[2]);
    }
    Plane.TextureName = Results[4];
    const std::vector<float> UInfo = __parseFloatArray(Results[5]);
    _ASSERTE(UInfo.size() == 4);
    Plane.TextureDirectionU = glm::vec3(UInfo[0], UInfo[1], UInfo[2]);
    Plane.TextureOffsetU = UInfo[3];
    const std::vector<float> VInfo = __parseFloatArray(Results[6]);
    _ASSERTE(VInfo.size() == 4);
    Plane.TextureDirectionV = glm::vec3(VInfo[0], VInfo[1], VInfo[2]);
    Plane.TextureOffsetV = VInfo[3];
    Plane.TextureRotation = __parseFloat(Results[7]);
    Plane.TextureScaleU = __parseFloat(Results[8]);
    Plane.TextureScaleV = __parseFloat(Results[9]);

    return Plane;
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