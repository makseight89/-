#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <string>

const char* filePath = "data.txt";
const size_t dataSize = 1000000;

std::wstring convertToWideString(const char* str) {
    size_t len = strlen(str) + 1;
    std::wstring wstr(len, L'\0');
    mbstowcs(&wstr[0], str, len);
    return wstr;
}

void processData(int* data, size_t size) {
    std::sort(data, data + size);
}

std::pair<double, double> benchmark() {
    int* data = new int[dataSize];
    auto start = std::chrono::high_resolution_clock::now();
    FILE* file = fopen(filePath, "rb");
    if (file) {
        fread(data, sizeof(int), dataSize, file);
        fclose(file);
    }
    
    std::thread t1(processData, data, dataSize);
    std::thread t2(processData, data, dataSize);
    t1.join();
    t2.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Traditional File Reading took " << duration.count() << " seconds.\n";
    std::pair<double, double> result;
    result.first = duration.count();

    start = std::chrono::high_resolution_clock::now();
    std::wstring wFilePath = convertToWideString(filePath);
    HANDLE hFile = CreateFileW(wFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping) {
        int* mappedData = (int*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
        if (mappedData) {
            std::thread t1(processData, mappedData, dataSize);
            std::thread t2(processData, mappedData, dataSize);
            t1.join();
            t2.join();
            UnmapViewOfFile(mappedData);
        }
        CloseHandle(hMapping);
    }
    CloseHandle(hFile);
    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    std::cout << "Memory-Mapped File Reading took " << duration.count() << " seconds.\n";
    result.second = duration.count();

    return result;
}