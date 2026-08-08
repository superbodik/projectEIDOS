#include "CrashTrace.h"

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "dbghelp.lib")

static std::string ModuleOf(HANDLE proc, DWORD64 addr) {
    IMAGEHLP_MODULE64 mod = { 0 };
    mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    if (SymGetModuleInfo64(proc, addr, &mod)) return mod.ModuleName;
    return "?";
}

std::string CaptureCrashTrace(_EXCEPTION_POINTERS* info) {
    std::ostringstream out;

    HANDLE proc = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    if (!SymInitialize(proc, nullptr, TRUE)) {
        out << "STACK: symbol engine unavailable (SymInitialize failed)\n";
        return out.str();
    }

    CONTEXT ctx = *((EXCEPTION_POINTERS*)info)->ContextRecord;

    STACKFRAME64 frame = { 0 };
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    out << "STACK (most recent call first):\n";

    char symBuf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)] = { 0 };
    SYMBOL_INFO* sym = (SYMBOL_INFO*)symBuf;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = MAX_SYM_NAME;

    for (int i = 0; i < 48; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, proc, thread, &frame, &ctx,
            nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        if (frame.AddrPC.Offset == 0) break;

        DWORD64 addr = frame.AddrPC.Offset;
        out << "  [" << std::setw(2) << i << "] ";

        DWORD64 disp = 0;
        if (SymFromAddr(proc, addr, &disp, sym)) out << sym->Name;
        else out << "0x" << std::hex << addr << std::dec;

        IMAGEHLP_LINE64 line = { 0 };
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisp = 0;
        if (SymGetLineFromAddr64(proc, addr, &lineDisp, &line))
            out << "   " << line.FileName << ":" << line.LineNumber;
        else
            out << "   (" << ModuleOf(proc, addr) << ")";

        out << "\n";
    }

    DWORD tid = GetCurrentThreadId();
    out << "THREAD: " << tid << "\n";

    SymCleanup(proc);
    return out.str();
}
#endif
