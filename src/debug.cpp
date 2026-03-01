#include "pch.h"
#include "debug.h"

static void LogToFile(const char* pszMessage) {
    char sPath[MAX_PATH];
    GetModuleFileNameA(nullptr, sPath, MAX_PATH);
    char* pSlash = strrchr(sPath, '\\');
    if (pSlash) *(pSlash + 1) = 0;
    strcat_s(sPath, MAX_PATH, "kinoko.log");
    FILE* f = nullptr;
    fopen_s(&f, sPath, "a");
    if (f) {
        fprintf(f, "%s\n", pszMessage);
        fflush(f);
        fclose(f);
    }
}

void DebugMessage(const char* pszFormat, ...) {
    char pszDest[1024];
    size_t cbDest = 1024 * sizeof(char);
    va_list argList;
    va_start(argList, pszFormat);
    StringCbVPrintfA(pszDest, cbDest, pszFormat, argList);
    OutputDebugStringA(pszDest);
    LogToFile(pszDest);
    va_end(argList);
}

void ErrorMessage(const char* pszFormat, ...) {
    char pszDest[1024];
    size_t cbDest = 1024 * sizeof(char);
    va_list argList;
    va_start(argList, pszFormat);
    StringCbVPrintfA(pszDest, cbDest, pszFormat, argList);
    MessageBox(nullptr, pszDest, "Error", MB_ICONERROR);
    va_end(argList);
}