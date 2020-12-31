#include "Interactor.h"
#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <set>
#include "IOGoldSrcMap.h"
#include "IOGoldSrcWad.h"
#include "IOImage.h"

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Vulkan Simple Render", nullptr, nullptr);
	CVulkanRenderer Renderer(pWindow);
	Renderer.getCamera()->setPos(glm::vec3(0.0f, 0.0f, 3.0f));
    Renderer.init(); 

    readMap("../data/test.map", Renderer);

	CInteractor Interactor(&Renderer);
	Interactor.bindEvent();

	std::chrono::milliseconds LastTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
		std::chrono::milliseconds CurrentTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		float DeltaTime = static_cast<float>((CurrentTimeStamp - LastTimeStamp).count()) / 1000.0f;
		Interactor.update(DeltaTime);
		Renderer.render();
		LastTimeStamp = CurrentTimeStamp;
	}
	Renderer.waitDevice();
	glfwDestroyWindow(pWindow);
	glfwTerminate();
	return 0;
}

void readObj(std::string vFileName, CVulkanRenderer& voRenderer)
{
    std::vector<float> VertexData;
    std::vector<uint32_t> IndexData;

    CIOObj Obj = CIOObj();
    Obj.read(vFileName);

    const std::vector<glm::vec3>& Vertices = Obj.getVertices();
    std::vector<glm::vec3> Normals = Obj.getNormalPerVertex();
    std::vector<glm::vec2> TexCoords = Obj.getRandomTexCoordPerVertex();
    for (size_t i = 0; i < Vertices.size(); ++i)
    {
        VertexData.push_back(Vertices[i].x);
        VertexData.push_back(Vertices[i].y);
        VertexData.push_back(Vertices[i].z);
        VertexData.push_back(1);
        VertexData.push_back(0);
        VertexData.push_back(0);
        VertexData.push_back(Normals[i].x);
        VertexData.push_back(Normals[i].y);
        VertexData.push_back(Normals[i].z);
        VertexData.push_back(TexCoords[i].x);
        VertexData.push_back(TexCoords[i].y);
    }
    const std::vector<SObjFace>& Faces = Obj.getFaces();
    for (const SObjFace& Face : Faces)
    {
        uint32_t Node1 = Face.Nodes[0].VectexId - 1;
        for (size_t k = 2; k < Face.Nodes.size(); k++)
        {
            uint32_t Node2 = Face.Nodes[k - 1].VectexId - 1;
            uint32_t Node3 = Face.Nodes[k].VectexId - 1;
            IndexData.push_back(Node1);
            IndexData.push_back(Node2);
            IndexData.push_back(Node3);
        }
    }
    voRenderer.setVertexData(VertexData);
    voRenderer.setIndexData(IndexData);
}

void readMap(std::string vFileName, CVulkanRenderer& voRenderer)
{
    std::vector<float> VertexData;
    std::vector<uint32_t> IndexData;

    CIOGoldSrcMap Map = CIOGoldSrcMap(vFileName);
    Map.read();
    std::vector<std::string> WadPaths = Map.getWadPaths();
    std::vector<CIOGoldsrcWad> Wads(WadPaths.size());
    
    for (size_t i = 0; i < WadPaths.size(); i++)
    {
        Wads[i].read(WadPaths[i]);
    }

    std::vector<CIOImage> TexImages;
    std::map<std::string, uint32_t> TexIndexMap;
    std::set<std::string> UsedTextureNames = Map.getUsedTextureNames();
    for (size_t i = 0; i < UsedTextureNames.size(); i++)
    {
        const std::string& TexName = UsedTextureNames[i];
        for (const CIOGoldsrcWad& Wad : Wads)
        {
            std::optional<size_t> Index = Wad.findTexture(TexName);
            if (Index.has_value())
            {
                uint32_t Width = 0, Height = 0;
                Wad.getTextureSize(Index.value(), Width, Height);

                CIOImage TexImage;
                TexImage.setImageSize(static_cast<int>(Width), static_cast<int>(Height));
                void* pData;
                Wad.getRawRGBAPixels(Index.value(), pData);
                TexImage.setData(pData);
                delete pData;
                TexImages.emplace_back(TexImage);
                TexIndexMap[TexName] = TexIndexMap.size();
                break;
            }
        }
        if (TexIndexMap.find(TexName) == TexIndexMap.end())
            TexIndexMap[TexName] = 0;
    }

    voRenderer.setVertexData(VertexData);
    voRenderer.setIndexData(IndexData);
}