#pragma once
#include <string>
#include <vector>
namespace Logger
{
    void Log(const std::string& message);
    const std::vector<std::string>& GetLogs();
    void ClearLogs();
}
