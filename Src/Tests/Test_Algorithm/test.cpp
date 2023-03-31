#include "pch.h"
#include "Maths.h"
#include "Visualizer.h"
#include <array>
#include <optional>

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