#include <windows.h>
#include <iostream>
#include <vector>

#define IN_PIPE_NAME "\\\\.\\pipe\\SortedPipe"

int main() {
    HANDLE hPipe = CreateFileA(
        IN_PIPE_NAME,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open pipe." << std::endl;
        return 1;
    }
    std::cout << "Receiving data from the sorting process..." << std::endl;

    std::vector<int> data(100);
    DWORD bytesRead;
    ReadFile(hPipe, data.data(), data.size() * sizeof(int), &bytesRead, NULL);
    CloseHandle(hPipe);

    for (int num : data) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    std::cout << "Data output process completed." << std::endl;
    return 0;
}