#pragma once
#include <string>

#ifdef _WIN32
struct _EXCEPTION_POINTERS;
std::string CaptureCrashTrace(_EXCEPTION_POINTERS* info);
#endif
