#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

class Logger {
public:
    Logger();
    ~Logger();

    void Init();
    void Log(const std::string& message);

    const std::vector<std::string>& GetHistory() const { return logHistory; }

private:
    std::ofstream logFile;
    std::vector<std::string> logHistory;
    std::string GetTimestamp(bool forFileName);
    std::mutex logMutex;
};
