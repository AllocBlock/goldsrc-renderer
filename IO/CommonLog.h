#pragma once
#include <functional>
#include <string>
#include <filesystem>

using LogFunc = std::function<void(std::filesystem::path)>;

void setGlobalLogFunc(LogFunc vLogFunc);
bool globalLog(std::filesystem::path vText);