#include <windows.h>
#include <vector>
#include <iostream>
#include <locale>
#include <codecvt>

struct HandleEntry {
    HANDLE Handle;
    DWORD Pid;
};

struct ZombieProcess {
    DWORD Pid;
    std::wstring Name;
    std::vector<HandleEntry> Handles;
};

bool IsZombieProcess(HANDLE processHandle) {
    DWORD result = WaitForSingleObject(processHandle, 0);
    return (result == WAIT_OBJECT_0);
}

std::string WStringToUTF8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static std::vector<ZombieProcess> zombieProcesses;

void SpawnZombieProcess() {
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;
    WCHAR name[] = L"test";
    if (CreateProcessW(NULL, name, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::wcout << L"Spawned zombie process with PID: " << pi.dwProcessId << std::endl;
        Sleep(2000);

        if (TerminateProcess(pi.hProcess, 100)) {
            std::wcout << L"Terminated process with PID: " << pi.dwProcessId << std::endl;
            ZombieProcess zp;
            zp.Pid = pi.dwProcessId;
            zp.Name = name;
            CloseHandle(pi.hThread);
            Sleep(500);

            if (IsZombieProcess(pi.hProcess)) {
                std::wcout << L"Process with PID: " << pi.dwProcessId << L" is a zombie process." << std::endl;
                zp.Handles.push_back({ pi.hProcess, pi.dwProcessId });
                zombieProcesses.push_back(zp);
            } else {
                std::wcout << L"Process with PID: " << pi.dwProcessId << L" is not a zombie process." << std::endl;
            }
        } else {
            std::wcerr << L"Failed to terminate process with PID: " << pi.dwProcessId << std::endl;
        }
    } else {
        DWORD error = GetLastError();
        std::wcerr << L"Failed to terminate process with PID: " << pi.dwProcessId << L". Error: " << error << std::endl;
    }
}

void TerminateZombieProcess(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process with PID: " << processId << std::endl;
        return;
    }

    if (CloseHandle(hProcess))
    {
        zombieProcesses.erase(std::remove_if(zombieProcesses.begin(), zombieProcesses.end(), [processId](const ZombieProcess& zp) {
            return zp.Pid == processId;
        }), zombieProcesses.end());
        std::wcout << L"Terminated zombie process with PID: " << processId << std::endl;
    } else {
        std::wcout << L"Failed to terminate process with PID: " << processId << std::endl;
    }
}
