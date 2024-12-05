#pragma once

#include <windows.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <thread>

class SharedMemory {
public:
    SharedMemory(size_t memorySize, size_t blockSize);
    ~SharedMemory();

    void writeBlock(size_t blockIndex, const void* data, size_t dataSize);
    void readBlock(size_t blockIndex, void* buffer, size_t bufferSize);

    double getBandwidth() const;
    double getActiveTime(size_t blockIndex) const;
    double getBlockedTime(size_t blockIndex) const;
    bool isBlockAccessed(size_t blockIndex) const;

private:
    size_t memorySize;
    size_t blockSize;
    size_t blockCount;
    HANDLE hFileMapping;
    void* pSharedMemory;
    std::vector<HANDLE> mutexes;
    std::vector<std::chrono::high_resolution_clock::time_point> activeStartTimes;
    std::vector<std::chrono::high_resolution_clock::time_point> blockedStartTimes;
    std::vector<double> activeTimes;
    std::vector<double> blockedTimes;
    std::vector<std::unique_ptr<std::atomic<bool>>> blockAccessed;
    std::atomic<double> totalBytesTransferred;
    std::chrono::high_resolution_clock::time_point startTime;
};

SharedMemory::SharedMemory(size_t memorySize, size_t blockSize)
    : memorySize(memorySize), blockSize(blockSize), totalBytesTransferred(0) {
    blockCount = memorySize / blockSize;
    hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, memorySize, NULL);
    pSharedMemory = MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, memorySize);

    for (size_t i = 0; i < blockCount; ++i) {
        HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
        mutexes.push_back(hMutex);
        activeTimes.push_back(0);
        blockedTimes.push_back(0);
        blockAccessed.push_back(std::make_unique<std::atomic<bool>>(false));
    }

    startTime = std::chrono::high_resolution_clock::now();
}

SharedMemory::~SharedMemory() {
    for (HANDLE hMutex : mutexes) {
        CloseHandle(hMutex);
    }
    UnmapViewOfFile(pSharedMemory);
    CloseHandle(hFileMapping);
}

void SharedMemory::writeBlock(size_t blockIndex, const void* data, size_t dataSize) {
    if (blockIndex >= blockCount || dataSize > blockSize) return;

    auto blockedStart = std::chrono::high_resolution_clock::now();
    WaitForSingleObject(mutexes[blockIndex], INFINITE);
    auto blockedEnd = std::chrono::high_resolution_clock::now();
    blockedTimes[blockIndex] += std::chrono::duration<double>(blockedEnd - blockedStart).count();

    *blockAccessed[blockIndex] = true;
    auto activeStart = std::chrono::high_resolution_clock::now();
    memcpy(static_cast<char*>(pSharedMemory) + blockIndex * blockSize, data, dataSize);
    Sleep(50); // Simulate processing time
    auto activeEnd = std::chrono::high_resolution_clock::now();
    activeTimes[blockIndex] += std::chrono::duration<double>(activeEnd - activeStart).count();
    *blockAccessed[blockIndex] = false;

    totalBytesTransferred += dataSize;
    ReleaseMutex(mutexes[blockIndex]);
}

void SharedMemory::readBlock(size_t blockIndex, void* buffer, size_t bufferSize) {
    if (blockIndex >= blockCount || bufferSize > blockSize) return;

    auto blockedStart = std::chrono::high_resolution_clock::now();
    WaitForSingleObject(mutexes[blockIndex], INFINITE);
    auto blockedEnd = std::chrono::high_resolution_clock::now();
    blockedTimes[blockIndex] += std::chrono::duration<double>(blockedEnd - blockedStart).count();

    *blockAccessed[blockIndex] = true;
    auto activeStart = std::chrono::high_resolution_clock::now();
    memcpy(buffer, static_cast<char*>(pSharedMemory) + blockIndex * blockSize, bufferSize);
    Sleep(50); // Simulate processing time
    auto activeEnd = std::chrono::high_resolution_clock::now();
    activeTimes[blockIndex] += std::chrono::duration<double>(activeEnd - activeStart).count();
    *blockAccessed[blockIndex] = false;

    ReleaseMutex(mutexes[blockIndex]);
}

double SharedMemory::getBandwidth() const {
    auto now = std::chrono::high_resolution_clock::now();
    double elapsedTime = std::chrono::duration<double>(now - startTime).count();
    return totalBytesTransferred / elapsedTime;
}

double SharedMemory::getActiveTime(size_t blockIndex) const {
    if (blockIndex >= blockCount) return 0;
    return activeTimes[blockIndex];
}

double SharedMemory::getBlockedTime(size_t blockIndex) const {
    if (blockIndex >= blockCount) return 0;
    return blockedTimes[blockIndex];
}

bool SharedMemory::isBlockAccessed(size_t blockIndex) const {
    if (blockIndex >= blockCount) return false;
    return *blockAccessed[blockIndex];
}





void writerThread(SharedMemory& sharedMemory, size_t blockIndex, const std::vector<int>& data, const std::atomic<bool>& running) {
    while (running) {
        for (const int& value : data) {
            if (!running) return;
            sharedMemory.writeBlock(blockIndex, &value, sizeof(value));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void readerThread(SharedMemory& sharedMemory, size_t blockIndex, size_t readCount, const std::atomic<bool>& running) {
    int buffer;
    while (running) {
        for (size_t i = 0; i < readCount; ++i) {
            if (!running) return;
            sharedMemory.readBlock(blockIndex, &buffer, sizeof(buffer));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}