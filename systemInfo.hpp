#ifndef SYSTEMINFO_HPP
#define SYSTEMINFO_HPP

#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <wbemidl.h>
#include <comdef.h>
#include <sysinfoapi.h>

#pragma comment(lib, "wbemuuid.lib")

class SystemInfo {
public:
    SystemInfo();
    ~SystemInfo();

    std::string getOSInfo();
    std::string getHardwareInfo();
    std::string getGPUInfo();
    std::string getNetworkInfo();
    std::string getCPUInfo();
    std::string getMemoryInfo();
    std::string getUptime();

private:
    void initializeCOM();
    void cleanupCOM();
    std::string queryWMI(const std::wstring& query, const std::wstring& property);

    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
};

SystemInfo::SystemInfo() {
    initializeCOM();
}

SystemInfo::~SystemInfo() {
    cleanupCOM();
}

void SystemInfo::initializeCOM() {
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        throw std::runtime_error("Failed to initialize COM library.");
    }

    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    if (FAILED(hres)) {
        CoUninitialize();
        throw std::runtime_error("Failed to initialize security.");
    }

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLoc
    );

    if (FAILED(hres)) {
        CoUninitialize();
        throw std::runtime_error("Failed to create IWbemLocator object.");
    }

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        throw std::runtime_error("Could not connect to WMI namespace.");
    }

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        throw std::runtime_error("Could not set proxy blanket.");
    }
}

void SystemInfo::cleanupCOM() {
    if (pSvc) {
        pSvc->Release();
    }
    if (pLoc) {
        pLoc->Release();
    }
    CoUninitialize();
}

std::string SystemInfo::queryWMI(const std::wstring& query, const std::wstring& property) {
    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );

    if (FAILED(hres)) {
        throw std::runtime_error("WMI query failed.");
    }

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    std::string result;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;
        hr = pclsObj->Get(property.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            result = _bstr_t(vtProp.bstrVal);
        }
        VariantClear(&vtProp);
        pclsObj->Release();
    }
    pEnumerator->Release();
    return result;
}

std::string SystemInfo::getOSInfo() {
    return queryWMI(L"SELECT * FROM Win32_OperatingSystem", L"Caption");
}

std::string SystemInfo::getHardwareInfo() {
    return queryWMI(L"SELECT * FROM Win32_ComputerSystem", L"Model");
}

std::string SystemInfo::getGPUInfo() {
    return queryWMI(L"SELECT * FROM Win32_VideoController", L"Name");
}

std::string SystemInfo::getNetworkInfo() {
    return queryWMI(L"SELECT * FROM Win32_NetworkAdapter", L"Name");
}

std::string SystemInfo::getCPUInfo() {
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);

    enum ProcessorArchitectureType {
        x64 = 9,
        arm = 5,
        arm64 = 12,
        ia64 = 6,
        x86 = 0,
        unknown = 0xFFFF
    };

    std::string cpuInfo = "Number of processors: " + std::to_string(siSysInfo.dwNumberOfProcessors) + "\n";
    cpuInfo += "Processor type: " + std::to_string(siSysInfo.dwProcessorType) + "\n";
    
    switch (siSysInfo.wProcessorArchitecture) {
    case ProcessorArchitectureType::x64:
        cpuInfo += "Processor architecture: x64\n";
        break;
    case ProcessorArchitectureType::arm:
        cpuInfo += "Processor architecture: ARM\n";
        break;
    case ProcessorArchitectureType::arm64:
        cpuInfo += "Processor architecture: ARM64\n";
        break;
    case ProcessorArchitectureType::ia64:
        cpuInfo += "Processor architecture: IA64\n";
        break;
    case ProcessorArchitectureType::x86:
        cpuInfo += "Processor architecture: x86\n";
        break;
    case ProcessorArchitectureType::unknown:
        cpuInfo += "Processor architecture: unknown\n";
        break;
    }

    return cpuInfo;
}

std::string SystemInfo::getMemoryInfo() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);

    std::string memoryInfo = "Total physical memory: " + std::to_string(statex.ullTotalPhys / 1024 / 1024) + " MB\n";
    memoryInfo += "Available physical memory: " + std::to_string(statex.ullAvailPhys / 1024 / 1024) + " MB\n";
    memoryInfo += "Total virtual memory: " + std::to_string(statex.ullTotalVirtual / 1024 / 1024) + " MB\n";
    memoryInfo += "Available virtual memory: " + std::to_string(statex.ullAvailVirtual / 1024 / 1024) + " MB\n";

    return memoryInfo;
}

std::string SystemInfo::getUptime() {
    ULONGLONG uptime = GetTickCount64();
    std::string uptimeInfo = "System uptime: " + std::to_string(uptime / 1000 / 60 / 60) + " hours\n";
    return uptimeInfo;
}

#endif // SYSTEMINFO_HPP