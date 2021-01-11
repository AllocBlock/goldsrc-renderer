#include "Interactor.h"
#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <set>
#include "IOGoldSrcMap.h"
#include "IOGoldSrcWad.h"
#include "IOImage.h"

SScene readObj(std::string vFileName);
SScene readMap(std::string vFileName);
bool findFile(std::string vFilePath, std::string vSearchDir, std::string& voFilePath);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Vulkan Simple Render", nullptr, nullptr);
	CVulkanRenderer Renderer(pWindow);
    SScene Scene = readMap("../data/cy_roating_grenade.map");
     //SScene Scene = readObj("../data/textured-cube.obj");
    Renderer.setScene(Scene);
    Renderer.init(); 

    std::shared_ptr<CInteractor> pInteractor = Renderer.getInteractor();
    pInteractor->getCamera()->setPos(glm::vec3(0.0f, 0.0f, 3.0f));

	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
		Renderer.render();
	}
	Renderer.waitDevice();
	glfwDestroyWindow(pWindow);
	glfwTerminate();
	return 0;
}

CIOImage generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize)
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

    CIOImage Grid;
    Grid.setImageSize(vNumCol * vCellSize, vNumRow * vCellSize);
    Grid.setData(pData);

    return Grid;
}

CIOImage generatePureColorTexture(unsigned char vBaseColor[3], size_t vSize)
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

    CIOImage Grid;
    Grid.setImageSize(vSize, vSize);
    Grid.setData(pData);

    return Grid;
}

SScene readObj(std::string vFileName)
{
    CIOObj Obj = CIOObj();
    Obj.read(vFileName);

    S3DObject ObjObject;
    ObjObject.TexIndex = 0;
    
    const std::vector<SObjFace>& Faces = Obj.getFaces();
    for (size_t i = 0; i < Faces.size(); ++i)
    {
        const SObjFace& Face = Faces[i];
        for (size_t k = 2; k < Face.Nodes.size(); ++k)
        {
            ObjObject.Vertices.emplace_back(Obj.getVertex(i, 0));
            ObjObject.Vertices.emplace_back(Obj.getVertex(i, k-1));
            ObjObject.Vertices.emplace_back(Obj.getVertex(i, k));
            ObjObject.Normals.emplace_back(Obj.getNormal(i, 0));
            ObjObject.Normals.emplace_back(Obj.getNormal(i, k - 1));
            ObjObject.Normals.emplace_back(Obj.getNormal(i, k));
            ObjObject.TexCoords.emplace_back(Obj.getTexCoord(i, 0));
            ObjObject.TexCoords.emplace_back(Obj.getTexCoord(i, k - 1));
            ObjObject.TexCoords.emplace_back(Obj.getTexCoord(i, k));
        }
    }
    for (size_t i = 0; i < ObjObject.Vertices.size(); ++i)
    {
        ObjObject.Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
    }

    std::vector<S3DObject> SceneObjects;
    SceneObjects.emplace_back(ObjObject);

    std::vector<CIOImage> TexImages;
    //TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    CIOImage Texture("../data/Tex2.png");
    Texture.read();
    TexImages.push_back(Texture);

    SScene Scene;
    Scene.Objects = SceneObjects;
    Scene.TexImages = TexImages;
    return Scene;
}

SScene readMap(std::string vFileName)
{
    CIOGoldSrcMap Map = CIOGoldSrcMap(vFileName);
    if (!Map.read())
        throw "file read failed";
    std::vector<std::string> WadPaths = Map.getWadPaths();
    std::vector<CIOGoldsrcWad> Wads(WadPaths.size());
    
    for (size_t i = 0; i < WadPaths.size(); ++i)
    {
        std::string RealWadPath;
        if (!findFile(WadPaths[i], "../data", RealWadPath))
            throw "can't find wad file " + WadPaths[i];
        Wads[i].read(RealWadPath);
    }

    SScene Scene;

    // find used textures, load and index them
    std::map<std::string, uint32_t> TexIndexMap;
    std::set<std::string> UsedTextureNames = Map.getUsedTextureNames();
    Scene.TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));
    TexIndexMap["TextureNotFound"] = 0;
    UsedTextureNames.insert("TextureNotFound");
    for (const std::string& TexName : UsedTextureNames)
    {
        for (const CIOGoldsrcWad& Wad : Wads)
        {
            std::optional<size_t> Index = Wad.findTexture(TexName);
            if (Index.has_value())
            {
                uint32_t Width = 0, Height = 0;
                Wad.getTextureSize(Index.value(), Width, Height);

                CIOImage TexImage;
                TexImage.setImageSize(static_cast<int>(Width), static_cast<int>(Height));
                void* pData = new unsigned char[static_cast<size_t>(4) * Width * Height];
                Wad.getRawRGBAPixels(Index.value(), pData);
                TexImage.setData(pData);
                delete[] pData;
                TexIndexMap[TexName] = Scene.TexImages.size();
                Scene.TexImages.emplace_back(TexImage);
                break;
            }
        }
        if (TexIndexMap.find(TexName) == TexIndexMap.end())
            TexIndexMap[TexName] = 0;
    }

    // group polygon by texture, one object per texture 
    Scene.Objects.resize(UsedTextureNames.size());
    for (size_t i = 0; i < Scene.Objects.size(); ++i)
        Scene.Objects[i].TexIndex = i;

    std::vector<CMapPolygon> Polygons = Map.getAllPolygons();
    
    for (CMapPolygon& Polygon : Polygons)
    {
        size_t TexIndex = TexIndexMap[Polygon.pPlane->TextureName];
        size_t TexWidth = Scene.TexImages[TexIndex].getImageWidth();
        size_t TexHeight = Scene.TexImages[TexIndex].getImageHeight();
        S3DObject& Object = Scene.Objects[TexIndex];
        uint32_t IndexStart = Object.Vertices.size();

        std::vector<glm::vec2> TexCoords = Polygon.getTexCoords(TexWidth, TexHeight);
        glm::vec3 Normal = Polygon.getNormal();

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

        for (size_t k = 2; k < Polygon.Vertices.size(); ++k)
        {
            Object.Vertices.emplace_back(Polygon.Vertices[0]);
            Object.Vertices.emplace_back(Polygon.Vertices[k-1]);
            Object.Vertices.emplace_back(Polygon.Vertices[k]);
            Object.Normals.emplace_back(Normal);
            Object.Normals.emplace_back(Normal);
            Object.Normals.emplace_back(Normal);
            Object.TexCoords.emplace_back(TexCoords[0]);
            Object.TexCoords.emplace_back(TexCoords[k-1]);
            Object.TexCoords.emplace_back(TexCoords[k]);
            Object.Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            Object.Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
            Object.Colors.emplace_back(glm::vec3(1.0, 1.0, 1.0));
        }
    }

    return Scene;
}

bool findFile(std::string vFilePath, std::string vSearchDir, std::string& voFilePath)
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
            voFilePath = SearchPath.string();
            return true;
        }
        else if (std::filesystem::exists(CombinedSearchPath))
        {
            voFilePath = CombinedSearchPath.string();
            return true;
        }
        CurDir = CurDir.parent_path();
    }

    return false;
}