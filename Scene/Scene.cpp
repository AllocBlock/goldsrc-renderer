﻿#include "Scene.h"

#include "IOGoldSrcMap.h"
#include "IOGoldsrcWad.h"
#include "IOGoldSrcBsp.h"
#include "IOObj.h"

#include <filesystem>

std::shared_ptr<CIOImage> generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize)
{
    unsigned char BaseColor1[3] = { 0, 0, 0 };
    unsigned char BaseColor2[3] = { 255, 0, 255 };
    size_t DataSize = vNumRow * vNumCol * vCellSize * vCellSize * 4;
    unsigned char* pData = new unsigned char[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        size_t GridRowIndex = (i / (vNumCol * vCellSize)) / vCellSize;
        size_t GridColIndex = (i % (vNumCol * vCellSize)) / vCellSize;

        unsigned char* pColor;
        if ((GridRowIndex + GridColIndex) % 2 == 0)
            pColor = BaseColor1;
        else
            pColor = BaseColor2;
        pData[i * 4] = pColor[0];
        pData[i * 4 + 1] = pColor[1];
        pData[i * 4 + 2] = pColor[2];
        pData[i * 4 + 3] = static_cast<unsigned char>(255);
    }

    // cout 
    /*for (size_t i = 0; i < vNumRow * vCellSize; i++)
    {
        for (size_t k = 0; k < vNumCol * vCellSize; k++)
        {
            std::cout << pData[(i * vNumCol * vCellSize + k) * 4] % 2 << " ";
        }
        std::cout << std::endl;
    }*/
    std::shared_ptr<CIOImage> pGrid = std::make_shared<CIOImage>();
    pGrid->setImageSize(vNumCol * vCellSize, vNumRow * vCellSize);
    pGrid->setData(pData);

    return pGrid;
}

std::shared_ptr<CIOImage> generatePureColorTexture(unsigned char vBaseColor[3], size_t vSize)
{
    size_t DataSize = vSize * vSize * 4;
    unsigned char* pData = new unsigned char[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        pData[i * 4] = vBaseColor[0];
        pData[i * 4 + 1] = vBaseColor[1];
        pData[i * 4 + 2] = vBaseColor[2];
        pData[i * 4 + 3] = static_cast<unsigned char>(255);
    }

    std::shared_ptr<CIOImage> pGrid = std::make_shared<CIOImage>();
    pGrid->setImageSize(vSize, vSize);
    pGrid->setData(pData);

    return pGrid;
}

bool findFile(std::filesystem::path vFilePath, std::filesystem::path vSearchDir, std::filesystem::path& voFilePath)
{
    std::filesystem::path CurPath(vFilePath);

    std::filesystem::path FullPath = std::filesystem::absolute(vFilePath);
    std::filesystem::path CurDir = FullPath.parent_path();
    while (!CurDir.empty() && CurDir != CurDir.parent_path())
    {
        std::filesystem::path SearchPath = std::filesystem::relative(FullPath, CurDir);
        std::filesystem::path CombinedSearchPath = std::filesystem::path(vSearchDir) / SearchPath;
        if (std::filesystem::exists(SearchPath))
        {
            voFilePath = SearchPath;
            return true;
        }
        else if (std::filesystem::exists(CombinedSearchPath))
        {
            voFilePath = CombinedSearchPath;
            return true;
        }
        CurDir = CurDir.parent_path();
    }

    return false;
}

SScene SceneReader::readBspFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    const float SceneScale = 1.0f / 64.0f;
    if (vProgressReportFunc) vProgressReportFunc(u8"[bsp]读取文件中");
    CIOGoldSrcBsp Bsp = CIOGoldSrcBsp(vFilePath);
    if (!Bsp.read())
        throw std::runtime_error(u8"文件解析失败");
    SScene Scene;

    const SBspLumps& Lumps = Bsp.getLumps();

    // read wads
    const std::vector<std::filesystem::path>& WadPaths = Lumps.m_LumpEntity.WadPaths;
    std::vector<CIOGoldsrcWad> Wads(WadPaths.size());

    for (size_t i = 0; i < WadPaths.size(); ++i)
    {
        std::filesystem::path RealWadPath;
        if (!findFile(WadPaths[i], "../data", RealWadPath))
            GlobalLogger::logStream() << u8"未找到或无法打开WAD文件：" << WadPaths[i];
        if (vProgressReportFunc) vProgressReportFunc(u8"[wad]读取" + RealWadPath.u8string() + u8"文件中");
        Wads[i].read(RealWadPath);
    }

    // load textures
    // iterate each texture in texture lump
    // if bsp contains its data, load it, otherwise find it in all wads
    // if found, load it, otherwise set mapper to 0
    if (vProgressReportFunc) vProgressReportFunc(u8"整理纹理中");
    std::map<std::string, uint32_t> TexNameToIndex;
    Scene.TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    TexNameToIndex["TextureNotFound"] = 0;
    for (size_t i = 0; i < Lumps.m_LumpTexture.Textures.size(); ++i)
    {
        const SBspTexture& BspTexture = Lumps.m_LumpTexture.Textures[i];
        if (BspTexture.IsDataInBsp)
        {
            std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
            void* pData = new unsigned char[static_cast<size_t>(4) * BspTexture.Width * BspTexture.Height];
            BspTexture.getRawRGBAPixels(pData);
            pTexImage->setData(pData);
            delete[] pData;
            TexNameToIndex[BspTexture.Name] = Scene.TexImages.size();
        }
        else
        {
            bool Found = false;
            for (const CIOGoldsrcWad& Wad : Wads)
            {
                std::optional<size_t> Index = Wad.findTexture(BspTexture.Name);
                if (Index.has_value())
                {
                    uint32_t Width = 0, Height = 0;
                    Wad.getTextureSize(Index.value(), Width, Height);

                    std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
                    pTexImage->setImageSize(static_cast<int>(Width), static_cast<int>(Height));
                    void* pData = new unsigned char[static_cast<size_t>(4) * Width * Height];
                    Wad.getRawRGBAPixels(Index.value(), pData);
                    pTexImage->setData(pData);
                    delete[] pData;
                    TexNameToIndex[BspTexture.Name] = Scene.TexImages.size();
                    Scene.TexImages.emplace_back(std::move(pTexImage));
                    Found = true;
                    break;
                }
            }
            if (!Found)
                TexNameToIndex[BspTexture.Name] = 0;
        }
    }

    // load faces
    size_t NumTexture = Scene.TexImages.size();
    Scene.Objects.resize(NumTexture);
    for (size_t i = 0; i < NumTexture; i++)
    {
        Scene.Objects[i] = std::make_shared<S3DObject>();
        Scene.Objects[i]->TexIndex = i;
    }
    
    for (const SBspFace& Face : Lumps.m_LumpFace.Faces)
    {
        // get vertices
        _ASSERTE(Face.PlaneIndex < Lumps.m_LumpPlane.Planes.size());
        const SBspPlane& Plane = Lumps.m_LumpPlane.Planes[Face.PlaneIndex];
        bool ReverseNormal = (Face.PlaneIndex != 0);
        glm::vec3 Normal = Plane.Normal.glmVec3();
        if (ReverseNormal) Normal *= -1;
        _ASSERTE(static_cast<size_t>(Face.FirstSurfedgeIndex) + Face.NumSurfedge <= Lumps.m_LumpSurfedge.Surfedges.size());

        std::vector<uint16_t> FaceVertexIndices;
        uint16_t LastVertexIndex2 = 0;
        for (uint16_t i = 0; i < Face.NumSurfedge; ++i)
        {
            uint16_t SurfedgeIndex = Face.FirstSurfedgeIndex + i;
            int16_t RawEdgeIndex = Lumps.m_LumpSurfedge.Surfedges[SurfedgeIndex];
            uint16_t VertexIndex1 = 0, VertexIndex2 = 0;
            if (RawEdgeIndex > 0)
            {
                uint16_t EdgeIndex = static_cast<uint16_t>(RawEdgeIndex);
                _ASSERTE(EdgeIndex < Lumps.m_LumpEdge.Edges.size());
                VertexIndex1 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[0];
                VertexIndex2 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[1];
            }
            else
            {
                uint16_t EdgeIndex = static_cast<uint16_t>(-RawEdgeIndex);
                _ASSERTE(EdgeIndex < Lumps.m_LumpEdge.Edges.size());
                VertexIndex1 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[1];
                VertexIndex2 = Lumps.m_LumpEdge.Edges[EdgeIndex].VertexIndices[0];
            }

            FaceVertexIndices.emplace_back(VertexIndex1);
            if ((i > 0 && LastVertexIndex2 != VertexIndex1) ||
                (i == Face.NumSurfedge - 1 && FaceVertexIndices[0] != VertexIndex2))
            {
                throw std::runtime_error(u8"BSP文件中，面的边未组成环");
            }
            LastVertexIndex2 = VertexIndex2;
        }

        std::vector<glm::vec3> FaceVertices;
        for (uint16_t FaceVertexIndex : FaceVertexIndices)
        {
            _ASSERTE(FaceVertexIndex < Lumps.m_LumpVertex.Vertices.size());
            SVec3 Vertex = Lumps.m_LumpVertex.Vertices[FaceVertexIndex];
            FaceVertices.emplace_back(Vertex.glmVec3() * SceneScale);
        }

        // find texcoords
        uint16_t TexInfoIndex = Face.TexInfoIndex;
        _ASSERTE(TexInfoIndex < Lumps.m_LumpTexInfo.TexInfos.size());
        const SBspTexInfo& TexInfo = Lumps.m_LumpTexInfo.TexInfos[TexInfoIndex];
        _ASSERTE(TexInfo.TextureIndex < Lumps.m_LumpTexture.Textures.size());
        const SBspTexture& BspTexture = Lumps.m_LumpTexture.Textures[TexInfo.TextureIndex];
        // original texture size
        size_t TexWidth = BspTexture.Width; 
        size_t TexHeight = BspTexture.Height;

        size_t TexIndex = TexNameToIndex[BspTexture.Name];
        // save data
        for (size_t i = 2; i < FaceVertices.size(); ++i)
        {
            Scene.Objects[TexIndex]->Vertices.emplace_back(FaceVertices[0]);
            Scene.Objects[TexIndex]->Vertices.emplace_back(FaceVertices[i-1]);
            Scene.Objects[TexIndex]->Vertices.emplace_back(FaceVertices[i]);
            Scene.Objects[TexIndex]->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            Scene.Objects[TexIndex]->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            Scene.Objects[TexIndex]->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            Scene.Objects[TexIndex]->Normals.emplace_back(Normal);
            Scene.Objects[TexIndex]->Normals.emplace_back(Normal);
            Scene.Objects[TexIndex]->Normals.emplace_back(Normal);
            Scene.Objects[TexIndex]->TexCoords.emplace_back(TexInfo.getTexCoord(FaceVertices[0] / SceneScale, TexWidth, TexHeight));
            Scene.Objects[TexIndex]->TexCoords.emplace_back(TexInfo.getTexCoord(FaceVertices[i-1] / SceneScale, TexWidth, TexHeight));
            Scene.Objects[TexIndex]->TexCoords.emplace_back(TexInfo.getTexCoord(FaceVertices[i] / SceneScale, TexWidth, TexHeight));
        }
    }

    return Scene;
}

SScene SceneReader::readMapFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    if (vProgressReportFunc) vProgressReportFunc(u8"[map]读取文件中");
    CIOGoldSrcMap Map = CIOGoldSrcMap(vFilePath);
    if (!Map.read())
        throw std::runtime_error(u8"文件解析失败");
    std::vector<std::filesystem::path> WadPaths = Map.getWadPaths();
    std::vector<CIOGoldsrcWad> Wads(WadPaths.size());

    for (size_t i = 0; i < WadPaths.size(); ++i)
    {
        std::filesystem::path RealWadPath;
        if (!findFile(WadPaths[i], "../data", RealWadPath))
            GlobalLogger::logStream() << u8"未找到或无法打开WAD文件：" << WadPaths[i];
        if (vProgressReportFunc) vProgressReportFunc(u8"[wad]读取" + RealWadPath.u8string() + u8"文件中");
        Wads[i].read(RealWadPath);
    }

    SScene Scene;

    if (vProgressReportFunc) vProgressReportFunc(u8"整理纹理中");
    // find used textures, load and index them
    std::map<std::string, uint32_t> TexNameToIndex;
    std::set<std::string> UsedTextureNames = Map.getUsedTextureNames();
    Scene.TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    TexNameToIndex["TextureNotFound"] = 0;
    UsedTextureNames.insert("TextureNotFound");
    for (const std::string& TexName : UsedTextureNames)
    {
        bool Found = false;
        for (const CIOGoldsrcWad& Wad : Wads)
        {
            std::optional<size_t> Index = Wad.findTexture(TexName);
            if (Index.has_value())
            {
                uint32_t Width = 0, Height = 0;
                Wad.getTextureSize(Index.value(), Width, Height);

                std::shared_ptr<CIOImage> pTexImage = std::make_shared<CIOImage>();
                pTexImage->setImageSize(static_cast<int>(Width), static_cast<int>(Height));
                void* pData = new unsigned char[static_cast<size_t>(4) * Width * Height];
                Wad.getRawRGBAPixels(Index.value(), pData);
                pTexImage->setData(pData);
                delete[] pData;
                TexNameToIndex[TexName] = Scene.TexImages.size();
                Scene.TexImages.emplace_back(std::move(pTexImage));
                Found = true;
                break;
            }
        }
        if (!Found)
            TexNameToIndex[TexName] = 0;
    }

    if (vProgressReportFunc) vProgressReportFunc(u8"生成场景中");
    // group polygon by texture, one object per texture 
    Scene.Objects.resize(UsedTextureNames.size());
    for (size_t i = 0; i < Scene.Objects.size(); ++i)
    {
        Scene.Objects[i] = std::make_shared<S3DObject>();
        Scene.Objects[i]->TexIndex = i;
    }

    std::vector<SMapPolygon> Polygons = Map.getAllPolygons();

    for (SMapPolygon& Polygon : Polygons)
    {
        size_t TexIndex = TexNameToIndex[Polygon.pPlane->TextureName];
        size_t TexWidth = Scene.TexImages[TexIndex]->getImageWidth();
        size_t TexHeight = Scene.TexImages[TexIndex]->getImageHeight();
        std::shared_ptr<S3DObject> pObject = Scene.Objects[TexIndex];
        uint32_t IndexStart = pObject->Vertices.size();

        std::vector<glm::vec2> TexCoords = Polygon.getTexCoords(TexWidth, TexHeight);
        glm::vec3 Normal = Polygon.getNormal();

        // indexed data
        /*Object.Vertices.insert(Object.Vertices.end(), Polygon.Vertices.begin(), Polygon.Vertices.end());
        Object.TexCoords.insert(Object.TexCoords.end(), TexCoords.begin(), TexCoords.end());
        for (size_t i = 0; i < Polygon.Vertices.size(); ++i)
        {
            Object.Colors.emplace_back(glm::vec3(1.0, 0.0, 0.0));
            Object.Normals.emplace_back(Normal);
        }

        for (size_t i = 2; i < Polygon.Vertices.size(); ++i)
        {
            Object.Indices.emplace_back(IndexStart);
            Object.Indices.emplace_back(IndexStart + i - 1);
            Object.Indices.emplace_back(IndexStart + i);
        }
        IndexStart += static_cast<uint32_t>(Polygon.Vertices.size());*/

        // non-indexed data
        for (size_t k = 2; k < Polygon.Vertices.size(); ++k)
        {
            pObject->Vertices.emplace_back(Polygon.Vertices[0]);
            pObject->Vertices.emplace_back(Polygon.Vertices[k - 1]);
            pObject->Vertices.emplace_back(Polygon.Vertices[k]);
            pObject->Normals.emplace_back(Normal);
            pObject->Normals.emplace_back(Normal);
            pObject->Normals.emplace_back(Normal);
            pObject->TexCoords.emplace_back(TexCoords[0]);
            pObject->TexCoords.emplace_back(TexCoords[k - 1]);
            pObject->TexCoords.emplace_back(TexCoords[k]);
            pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            pObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        }
    }
    if (vProgressReportFunc) vProgressReportFunc(u8"完成");
    return Scene;
}

SScene SceneReader::readObjFile(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    if (vProgressReportFunc) vProgressReportFunc(u8"[obj]读取文件中");
    CIOObj Obj = CIOObj();
    Obj.read(vFilePath);

    if (vProgressReportFunc) vProgressReportFunc(u8"生成场景中");
    std::shared_ptr<S3DObject> pObjObject = std::make_shared<S3DObject>();
    pObjObject->TexIndex = 0;

    const std::vector<SObjFace>& Faces = Obj.getFaces();
    for (size_t i = 0; i < Faces.size(); ++i)
    {
        const SObjFace& Face = Faces[i];
        for (size_t k = 2; k < Face.Nodes.size(); ++k)
        {
            pObjObject->Vertices.emplace_back(Obj.getVertex(i, 0));
            pObjObject->Vertices.emplace_back(Obj.getVertex(i, k - 1));
            pObjObject->Vertices.emplace_back(Obj.getVertex(i, k));
            pObjObject->Normals.emplace_back(Obj.getNormal(i, 0));
            pObjObject->Normals.emplace_back(Obj.getNormal(i, k - 1));
            pObjObject->Normals.emplace_back(Obj.getNormal(i, k));
            pObjObject->TexCoords.emplace_back(Obj.getTexCoord(i, 0));
            pObjObject->TexCoords.emplace_back(Obj.getTexCoord(i, k - 1));
            pObjObject->TexCoords.emplace_back(Obj.getTexCoord(i, k));
        }
    }
    for (size_t i = 0; i < pObjObject->Vertices.size(); ++i)
    {
        pObjObject->Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
    }

    SScene Scene;
    Scene.Objects.emplace_back(pObjObject);
    Scene.TexImages.emplace_back(generateBlackPurpleGrid(4, 4, 16));
    //CIOImage Texture("../data/Tex2.png");
    //Texture.read();
    //TexImages.push_back(Texture);

    return Scene;
}