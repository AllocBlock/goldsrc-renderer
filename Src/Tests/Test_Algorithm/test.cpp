#include "pch.h"
#include "Maths.h"
#include "Visualizer.h"
#include <array>

//#define _DEBUG_VISUALIZE

TEST(Algorithm, RayCastTriangle) {
    std::array<glm::vec3, 3> Triangle = {
        glm::vec3(5.207108f, -1.792894f, -3.949748f),
        glm::vec3(5.207108f, -1.792894f, 5.949748f),
        glm::vec3(-1.792894f, 5.207108f, 1),
    };

    std::vector<std::pair<glm::vec3, glm::vec3>> Rays =
    {
        { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
        { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f) },
        { glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) },
        { glm::vec3(1.0f, 1.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
        { glm::vec3(2.0f, 2.0f, 3.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
        { glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(-3.0f, -3.0f, -3.0f) },
        { glm::vec3(1.0f, 1.0f, 3.0f), glm::vec3(-2.0f, -3.0f, 1.0f) },
    };

    std::vector<bool> ExpectedResults = {
        false, false, true, false, true, true, false
    };

    ASSERT_EQ(ExpectedResults.size(), Rays.size());

    float t, u, v;
#ifdef _DEBUG_VISUALIZE
    std::vector<std::optional<glm::vec3>> IntersectedSet;
#endif

    for (size_t i = 0; i < Rays.size(); ++i)
    {
        const auto& Ray = Rays[i];
        bool Intersected = Math::intersectRayTriangle(Ray.first, Ray.second, Triangle[0], Triangle[1], Triangle[2], t, u, v);
        bool ExpectResult = ExpectedResults[i];
        EXPECT_EQ(Intersected, ExpectResult);

#ifdef _DEBUG_VISUALIZE
        if (Intersected)
            IntersectedSet.emplace_back(Ray.first + Ray.second * t);
        else
            IntersectedSet.emplace_back(std::nullopt);
#endif

    }

#ifdef _DEBUG_VISUALIZE
    CVisualizer Visualizer;
    Visualizer.init();
    for (size_t i = 0; i < Rays.size(); ++i)
    {
        const auto& Ray = Rays[i];
        bool Intersected = IntersectedSet[i].has_value();
        glm::vec3 RayColor = Intersected ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        Visualizer.addTriangle({ Triangle[0], Triangle[1], Triangle[2] }, glm::vec3(1, 1, 1));
        Visualizer.addLine({ Ray.first, Ray.first + Ray.second * 10.0F }, RayColor);
        if (Intersected)
        {
            Visualizer.addPoint(IntersectedSet[i].value(), glm::vec3(0, 1, 0));
        }
    }
    Visualizer.start();
#endif
}


TEST(Algorithm, RayCastAABB) {
    SAABB AABB = SAABB(glm::vec3(0.5, 0.5, 0.5), glm::vec3(2.0, 2.0, 2.0));

    std::vector<std::pair<glm::vec3, glm::vec3>> Rays =
    {
        { glm::vec3(-2, -2, -2), glm::vec3(2, 2, 2) },
        { glm::vec3(0, 0, 0), glm::vec3(0, 1, 0) },
        { glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0) },
        { glm::vec3(1, 1, 1), glm::vec3(1, 1, 0) },
        { glm::vec3(0.4, 1, 1), glm::vec3(-1, 0, 0) },
};

    std::vector<bool> ExpectedResults = {
        true, false, false, true, false
    };

    ASSERT_EQ(ExpectedResults.size(), Rays.size());

    float NearT, FarT;
    for (size_t i = 0; i < Rays.size(); ++i)
    {
        const auto& Ray = Rays[i];
        bool Intersected = Math::intersectRayAABB(Ray.first, Ray.second, AABB, true, NearT, FarT);
        bool ExpectResult = ExpectedResults[i];
        EXPECT_EQ(Intersected, ExpectResult);
    }
}


TEST(Algorithm, DirectedGraphLoopTest) {
    Math::CDirectedGraph Graph1;
    Graph1.addNode(0);
    EXPECT_FALSE(Graph1.hasLoop());
    Graph1.addEdge(0, 0);
    EXPECT_TRUE(Graph1.hasLoop());

    Math::CDirectedGraph Graph2;
    Graph2.addNode(0);
    Graph2.addNode(1);
    EXPECT_FALSE(Graph2.hasLoop());
    Graph2.addEdge(0, 1);
    EXPECT_FALSE(Graph2.hasLoop());
    Graph2.addEdge(1, 0);
    EXPECT_TRUE(Graph2.hasLoop());

    Math::CDirectedGraph Graph3;
    Graph3.addNode(0);
    Graph3.addNode(1);
    Graph3.addNode(2);
    EXPECT_FALSE(Graph3.hasLoop());
    Graph3.addEdge(0, 1);
    EXPECT_FALSE(Graph3.hasLoop());
    Graph3.addEdge(1, 2);
    EXPECT_FALSE(Graph3.hasLoop());
    Graph3.addEdge(2, 0);
    EXPECT_TRUE(Graph3.hasLoop());


    Math::CDirectedGraph Graph4;
    Graph4.addNode(0);
    Graph4.addNode(1);
    Graph4.addNode(2);
    Graph4.addNode(3);
    EXPECT_FALSE(Graph4.hasLoop());
    Graph4.addEdge(0, 1);
    EXPECT_FALSE(Graph4.hasLoop());
    Graph4.addEdge(1, 2);
    EXPECT_FALSE(Graph4.hasLoop());
    Graph4.addEdge(2, 3);
    EXPECT_FALSE(Graph4.hasLoop());
    Graph4.addEdge(3, 1);
    EXPECT_TRUE(Graph4.hasLoop());
}