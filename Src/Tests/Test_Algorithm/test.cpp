#include "pch.h"
#include "Maths.h"
#include <array>



TEST(Algorithm, RayCast) {

    std::vector<std::array<glm::vec3, 3>> Triangles = {
        {glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
    };

    std::vector<bool> DoesIntersects = {
        true
    };

    // cube
    // create plane and vertex buffer
    std::vector<glm::vec3> VertexSet =
    {
        { 1.0,  1.0,  1.0}, // 0
        {-1.0,  1.0,  1.0}, // 1
        {-1.0,  1.0, -1.0}, // 2
        { 1.0,  1.0, -1.0}, // 3
        { 1.0, -1.0,  1.0}, // 4
        {-1.0, -1.0,  1.0}, // 5
        {-1.0, -1.0, -1.0}, // 6
        { 1.0, -1.0, -1.0}, // 7
    };

    for (auto& Vertex : VertexSet)
    {
        Vertex = vSize * Vertex + vOrigin;
    }

    const std::vector<size_t> IndexSet =
    {
        4, 1, 0, 4, 5, 1, // +z
        3, 6, 7, 3, 2, 6, // -z
        0, 2, 3, 0, 1, 2, // +y
        5, 7, 6, 5, 4, 7, // -y
        4, 3, 7, 4, 0, 3, // +x
        1, 6, 2, 1, 5, 6, // -x
    };

    const std::vector<glm::vec3> NormalSet =
    {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, -1.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
    };


    glm::vec3 Origin = glm::vec3(0, 0, 1);
    glm::vec3 Direction = glm::vec3(0, 0, -1);

    ASSERT_EQ(Triangles.size(), DoesIntersects.size());

    float t, u, v;
    for (size_t i = 0; i < Triangles.size(); ++i)
    {
        const auto& Triangle = Triangles[i];
        bool Intersected = Math::intersectRayTriangle(Origin, Direction, Triangle[0], Triangle[1], Triangle[2], t, u, v);
        bool ExpectResult = DoesIntersects[i];
        EXPECT_EQ(Intersected, ExpectResult);
    }
}