#pragma once
#include <string>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace UI
{
    void text(std::string vName, bool vWarp = false);
    void bulletText(std::string vName);
    bool input(std::string vName, float& vioValue, float vStep = 0.01f);
    bool drag(std::string vName, float& vioValue, float vStep = 0.01f, float vMin = 0.0f, float vMax = 0.0f);
    bool drag(std::string vName, glm::vec3& vioValue, float vStep = 0.01f, float vMin = 0.0f, float vMax = 0.0f);
    bool slider(std::string vName, float& vioValue, float vMin = 0.0f, float vMax = 0.0f, std::string vFormat = "");
    bool inputColor(std::string vName, glm::vec3& vioColor);
    bool inputColor(std::string vName, glm::vec4& vioColor);

    bool toggle(std::string vName, bool& vioValue);
    bool button(std::string vName);
    bool collapse(std::string vName, bool vDefaultOpen = false);
    void beginGroup();
    void endGroup();
    void sameLine();
    void indent(float vWidth);
    void unindent();
    void spacing();
    void split();
    bool isUsingMouse();
    bool isUsingKeyboard();
};

