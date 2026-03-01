#include "config.h"
#include "debug.h"
#include <windows.h>
#include <detours.h>

static bool IsWine() {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    return hNtdll && GetProcAddress(hNtdll, "wine_get_version") != nullptr;
}

static bool LaunchWithDetours(LPSTR lpCmdLine, PROCESS_INFORMATION* pi) {
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(pi, sizeof(*pi));
    return DetourCreateProcessWithDllExA("MapleStory.exe", lpCmdLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, pi, CONFIG_DLL_NAME, NULL);
}

static bool LaunchWithManualInject(LPSTR lpCmdLine, PROCESS_INFORMATION* pi) {
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(pi, sizeof(*pi));

    // Create process suspended
    if (!CreateProcessA("MapleStory.exe", lpCmdLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, pi)) {
        return false;
    }

    // Get full path to our DLL
    char dllPath[MAX_PATH];
    GetFullPathNameA(CONFIG_DLL_NAME, MAX_PATH, dllPath, NULL);

    // Allocate memory in target process for DLL path
    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remoteMem = VirtualAllocEx(pi->hProcess, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        TerminateProcess(pi->hProcess, 1);
        return false;
    }

    // Write DLL path to target process
    if (!WriteProcessMemory(pi->hProcess, remoteMem, dllPath, pathLen, NULL)) {
        VirtualFreeEx(pi->hProcess, remoteMem, 0, MEM_RELEASE);
        TerminateProcess(pi->hProcess, 1);
        return false;
    }

    // Get LoadLibraryA address
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");

    // Create remote thread to load our DLL
    HANDLE hThread = CreateRemoteThread(pi->hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, remoteMem, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(pi->hProcess, remoteMem, 0, MEM_RELEASE);
        TerminateProcess(pi->hProcess, 1);
        return false;
    }

    // Wait for DLL to load
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(pi->hProcess, remoteMem, 0, MEM_RELEASE);

    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    PROCESS_INFORMATION pi;
    bool success;

    if (IsWine()) {
        success = LaunchWithManualInject(lpCmdLine, &pi);
    } else {
        success = LaunchWithDetours(lpCmdLine, &pi);
    }

    if (!success) {
        ErrorMessage("Could not start MapleStory.exe [%d]", GetLastError());
        return 1;
    }

    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwExitCode;
    if (!GetExitCodeProcess(pi.hProcess, &dwExitCode)) {
        ErrorMessage("GetExitCodeProcess failed [%d]", GetLastError());
        return 1;
    }
    return 0;
}
