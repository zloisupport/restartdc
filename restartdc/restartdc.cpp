#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <optional>
#include <string>
#include <functional>


using Predicate = std::function<bool(DWORD, const wchar_t*)>;
using OptionalWindow = std::optional<HWND>;


void printError(const TCHAR* msg);
bool isTargetProcess(DWORD processId, const wchar_t* targetExeName);
OptionalWindow findWindowByClass(const wchar_t* className);
bool findUserWindow(const wchar_t* className, const wchar_t* targetExeName, Predicate isTargetProcess);
void startProcess(const std::string& processName);


int main()
{
  
    Predicate isCalcProcess = isTargetProcess;

   
    bool windowClosed = findUserWindow(L"TTOTAL_CMD", L"doublecmd.exe", isCalcProcess);

    if (windowClosed)
    {
        startProcess("doublecmd.exe");
    }

    return 0;
}

OptionalWindow findWindowByClass(const wchar_t* className)
{
    HWND hwnd = FindWindowW(className, NULL);
    if (hwnd != NULL)
    {
        return hwnd;
    }
    return std::nullopt;
}


bool findUserWindow(const wchar_t* className, const wchar_t* targetExeName, Predicate isTargetProcess)
{
    auto window = findWindowByClass(className);
    if (window)
    {
        DWORD processId = 0;
        GetWindowThreadProcessId(*window, &processId);

        if (isTargetProcess(processId, targetExeName))
        {
            PostMessage(*window, WM_CLOSE, 0, 0);
            return true;
        }
    }
    return false;
}


bool isTargetProcess(DWORD processId, const wchar_t* targetExeName)
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        printf("Failed to create process snapshot.\n");
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    bool result = false;
    if (Process32FirstW(hProcessSnap, &pe32))
    {
        do
        {
            if (pe32.th32ProcessID == processId && _wcsicmp(pe32.szExeFile, targetExeName) == 0)
            {
                result = true;
                break;
            }
        } while (Process32NextW(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return result;
}


void startProcess(const std::string& processName)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(
        NULL,
        (LPSTR)processName.c_str(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi))
    {
        printf("Failed to start process: %s\n", processName.c_str());
        printf("Error: %lu\n", GetLastError());
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("Process started successfully: %s\n", processName.c_str());
}


void printError(const TCHAR* msg)
{
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, eNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        sysMsg, 256, NULL);

    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do { *p-- = 0; } while ((p >= sysMsg) &&
        ((*p == '.') || (*p < 33)));

    _tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}
