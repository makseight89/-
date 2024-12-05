#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

#define PIPE_NAME "\\\\.\\pipe\\DataPipe"
#define BUFFER_SIZE 1024

void GenerateData(std::vector<int>& data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data.push_back(rand() % 100);
    }
}

int main() {
    HANDLE hPipe = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        BUFFER_SIZE,
        BUFFER_SIZE,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create named pipe." << std::endl;
        return 1;
    }

    std::vector<int> data;
    GenerateData(data, 100);
    std::cout << "Data: ";
    for (int num : data) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    if (!ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
        std::cerr << "Failed to connect to the sorting process." << std::endl;
        CloseHandle(hPipe);
        return 1;
    }

    std::cout << "Sending data to the sorting process..." << std::endl;

    DWORD bytesWritten;
    if (WriteFile(hPipe, data.data(), data.size() * sizeof(int), &bytesWritten, NULL)) {
        std::cout << "Data sent successfully." << std::endl;
    } else {
        std::cerr << "Failed to write data to the pipe." << std::endl;
    }

    CloseHandle(hPipe);
    std::cout << "Data generation process completed." << std::endl;
    return 0;
}