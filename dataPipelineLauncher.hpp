#ifndef DATA_PIPELINE_LAUNCHER_HPP
#define DATA_PIPELINE_LAUNCHER_HPP

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

bool LaunchProcess(const std::string& processName, PROCESS_INFORMATION& pi) {
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process.
    if (!CreateProcessA(
            NULL,
            const_cast<char*>(processName.c_str()),
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi
        )
    ) {
        std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
        return false;
    }

    std::cout << "Launched " << processName << " with pid " << pi.dwProcessId << std::endl;
    return true;
}

void LaunchDataPipeline() {
    std::vector<PROCESS_INFORMATION> processes(3);

    if (!LaunchProcess("dataGeneration.exe", processes[0])) {
        std::cerr << "Failed to launch dataGeneration.exe\n";
        return;
    }

    Sleep(500);

    if (!LaunchProcess("dataSorting.exe", processes[1])) {
        std::cerr << "Failed to launch dataSorting.exe\n";
        return;
    }

    Sleep(500);

    if (!LaunchProcess("dataOutput.exe", processes[2])) {
        std::cerr << "Failed to launch dataOutput.exe\n";
        return;
    }

    // Wait for all processes to complete
    for (const auto& pi : processes) {
        WaitForSingleObject(pi.hProcess, 1000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

#endif // DATA_PIPELINE_LAUNCHER_HPP