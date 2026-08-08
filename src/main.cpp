#include "Core/EidosEngine.h"
#include "Core/CrashTrace.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOGDI
#define NOUSER
#include <windows.h>
#endif

void WriteCrashLog(const std::string& errorType, const std::string& details) {
    std::ofstream file("CRASH_REPORT.txt", std::ios::app);
    if (file.is_open()) {
        std::time_t now = std::time(nullptr);
        char timeBuf[100];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        file << "========================================\n";
        file << "TIME: " << timeBuf << "\n";
        file << "ERROR: " << errorType << "\n";
        file << "DETAILS: " << details << "\n";
        file << "========================================\n\n";
        file.flush();
        file.close();
    }
    std::cerr << "[CRASH] " << errorType << ": " << details << std::endl;
}

#ifdef _WIN32
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo) {
    DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;
    std::string errName;

    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: errName = "ACCESS VIOLATION (Reading/Writing to Another's Memory)"; break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: errName = "ARRAY BOUNDS EXCEEDED (Array index out of bounds)"; break;
    case EXCEPTION_BREAKPOINT: errName = "BREAKPOINT"; break;
    case EXCEPTION_DATATYPE_MISALIGNMENT: errName = "DATATYPE MISALIGNMENT"; break;
    case EXCEPTION_FLT_DENORMAL_OPERAND: errName = "FLOAT DENORMAL OPERAND"; break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: errName = "FLOAT DIVIDE BY ZERO"; break;
    case EXCEPTION_FLT_INEXACT_RESULT: errName = "FLOAT INEXACT RESULT"; break;
    case EXCEPTION_FLT_INVALID_OPERATION: errName = "FLOAT INVALID OPERATION"; break;
    case EXCEPTION_FLT_OVERFLOW: errName = "FLOAT OVERFLOW"; break;
    case EXCEPTION_FLT_STACK_CHECK: errName = "FLOAT STACK CHECK"; break;
    case EXCEPTION_FLT_UNDERFLOW: errName = "FLOAT UNDERFLOW"; break;
    case EXCEPTION_ILLEGAL_INSTRUCTION: errName = "ILLEGAL INSTRUCTION (Плохой код процессора)"; break;
    case EXCEPTION_IN_PAGE_ERROR: errName = "IN PAGE ERROR (Ошибка памяти)"; break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO: errName = "INT DIVIDE BY ZERO"; break;
    case EXCEPTION_INT_OVERFLOW: errName = "INT OVERFLOW"; break;
    case EXCEPTION_INVALID_DISPOSITION: errName = "INVALID DISPOSITION"; break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: errName = "NONCONTINUABLE EXCEPTION"; break;
    case EXCEPTION_PRIV_INSTRUCTION: errName = "PRIV INSTRUCTION"; break;
    case EXCEPTION_SINGLE_STEP: errName = "SINGLE STEP"; break;
    case EXCEPTION_STACK_OVERFLOW: errName = "STACK OVERFLOW (Переполнение стека)"; break;
    default: errName = "UNKNOWN EXCEPTION: " + std::to_string(code); break;
    }

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        ULONG_PTR* info = pExceptionInfo->ExceptionRecord->ExceptionInformation;
        bool write = info[0] == 1;
        unsigned long long address = (unsigned long long)info[1];
        errName += "\nAttempted to " + std::string(write ? "WRITE" : "READ");
        errName += " address: " + std::to_string(address);
    }

    std::string trace = CaptureCrashTrace(pExceptionInfo);
    WriteCrashLog("SYSTEM CRASH (Windows SEH)",
        errName + std::string("\n") + trace);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

    try {
        EidosEngine app(1280, 720, "Project EIDOS: Alpha");

        while (!app.ShouldClose()) {
            app.Update();
            app.Render();
        }
    }
    catch (const std::exception& e) {
        WriteCrashLog("Standard Exception", e.what());
        return -1;
    }
    catch (...) {
        WriteCrashLog("Unknown Error", "Something went wrong.");
        return -1;
    }

    return 0;
}
