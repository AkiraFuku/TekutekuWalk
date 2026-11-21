#include "Logger.h"
#include <Windows.h>
#include <mutex>

namespace Logger
{
    namespace {
        std::vector<std::string> g_logs;
        std::mutex g_logMutex;
        constexpr size_t kMaxLogs = 1000;
    }

    void Log(const std::string& message) {
        OutputDebugStringA(message.c_str());
        std::lock_guard<std::mutex> lock(g_logMutex);
        if (g_logs.size() >= kMaxLogs) {
            g_logs.erase(g_logs.begin());
        }
        g_logs.push_back(message);
    }

    const std::vector<std::string>& GetLogs() {
        return g_logs;
    }

    void ClearLogs() {
        std::lock_guard<std::mutex> lock(g_logMutex);
        g_logs.clear();
    }
}
