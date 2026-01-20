#include "Logger.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

Logger::Logger() {}

Logger::~Logger() {
    if (logFile.is_open()) {
        Log("=== SESSION ENDED ===");
        logFile.close();
    }
}

void Logger::Init() {
    if (!fs::exists("logs")) fs::create_directories("logs");
    std::string filename = "logs/log_" + GetTimestamp(true) + ".txt";
    logFile.open(filename);

    if (logFile.is_open()) {
        Log("=== ENGINE STARTED ===");
        Log("Log file created: " + filename);
    }
    else {
        std::cerr << "[LOGGER ERROR] Failed to create log file!" << std::endl;
    }
}

void Logger::Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::string finalMsg = message; 

    logHistory.push_back(finalMsg);
    if (logHistory.size() > 500) logHistory.erase(logHistory.begin());

    if (logFile.is_open()) {
        logFile << GetTimestamp(false) << message << std::endl;
        logFile.flush();
    }

    std::cout << GetTimestamp(false) << message << std::endl;
}

std::string Logger::GetTimestamp(bool forFileName) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    if (forFileName) ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
    else ss << std::put_time(std::localtime(&in_time_t), "[%H:%M:%S] ");
    return ss.str();
}