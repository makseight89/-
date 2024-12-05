#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>

#define IN_PIPE_NAME "\\\\.\\pipe\\DataPipe"
#define OUT_PIPE_NAME "\\\\.\\pipe\\SortedPipe"
#define BUFFER_SIZE 1024

int main() {
    HANDLE hInputPipe = CreateFileA(
        IN_PIPE_NAME,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hInputPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open input pipe." << std::endl;
        return 1;
    }
    std::cout << "Receiving data from the generator..." << std::endl;

    std::vector<int> data(100);
    DWORD bytesRead;
    ReadFile(hInputPipe, data.data(), data.size() * sizeof(int), &bytesRead, NULL);
    CloseHandle(hInputPipe);

    std::sort(data.begin(), data.end());

    HANDLE hOutputPipe = CreateNamedPipeA(
        OUT_PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        BUFFER_SIZE,
        BUFFER_SIZE,
        0,
        NULL
    );

    if (hOutputPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create output pipe." << std::endl;
        return 1;
    }
    std::cout << "Waiting for the output process to connect..." << std::endl;

    if (ConnectNamedPipe(hOutputPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
        DWORD bytesWritten;
        WriteFile(hOutputPipe, data.data(), data.size() * sizeof(int), &bytesWritten, NULL);
    } else {
        std::cerr << "Failed to connect to the output process." << std::endl;
    }

    CloseHandle(hOutputPipe);
    std::cout << "Data sorting process completed." << std::endl;
    return 0;
}