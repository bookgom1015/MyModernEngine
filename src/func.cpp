#include "func.h"

#include "struct.h"

#include <mutex>
#include <vector>
#include <iostream>

#include "Asset.hpp"

namespace {
    const char* const STR_FAIL_GET_PROC_INFO = "Failed to retrieve processor information.";
    const char* const STR_FAIL_GET_BUFF_SIZE = "Failed to get required buffer size.";
    const char* const STR_FAIL_GET_MEM_STAT = "Failed to get memory status";
}

LogFile Logger::mLogFile{};

BOOL Logger::Initialize(LPCWSTR filePath) {
	mLogFile.Handle = CreateFile(
		filePath,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD writtenBytes = 0;
	WORD bom = 0xFEFF;

	return WriteFile(mLogFile.Handle, &bom, 2, &writtenBytes, NULL);
}

void Logger::LogFn(const std::string& msg) {
	std::wstring wstr = StrToWStr(msg);

	DWORD writtenBytes = 0;
	{
		std::lock_guard<std::mutex> lock(mLogFile.Mutex);

		WriteFile(
			mLogFile.Handle,
			wstr.c_str(),
			static_cast<DWORD>(wstr.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
#ifdef _DEBUG
    std::cout << msg;
#endif
}

void Logger::LogFn(const std::wstring& msg) {
	DWORD writtenBytes = 0;
	{
		std::lock_guard<std::mutex> lock(mLogFile.Mutex);

		WriteFile(
            mLogFile.Handle,
			msg.c_str(),
			static_cast<DWORD>(msg.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
#ifdef _DEBUG
    std::cout << WStrToStr(msg);
#endif
}

BOOL Logger::SetTextToWnd(HWND hWnd, LPCWSTR text) {
	return SetWindowTextW(hWnd, text);
}

BOOL Logger::AppendTextToWnd(HWND hWnd, LPCWSTR text) {
	const INT length = GetWindowTextLengthW(hWnd) + lstrlenW(text) + 1;

	std::vector<WCHAR> buffer(length);

	GetWindowTextW(hWnd, buffer.data(), length);
	wcscat_s(buffer.data(), length, text);
	SetWindowTextW(hWnd, buffer.data());

	return TRUE;
}

bool HWInfo::ProcessorInfo(Processor* const pInfo) {
    CheckReturn(GetProcessorName(pInfo));
    CheckReturn(GetInstructionSupport(pInfo));
    CheckReturn(GetCoreInfo(pInfo));
    CheckReturn(GetSystemMemoryInfo(pInfo));

    return true;
}

bool HWInfo::GetProcessorName(Processor* const pInfo) {
    INT CPUInfo[4] { -1 };
    CHAR CPUBrandString[0x40] { 0 };

    // Processor's name
    __cpuid(CPUInfo, 0x80000002);
    memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));

    __cpuid(CPUInfo, 0x80000003);
    memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));

    __cpuid(CPUInfo, 0x80000004);
    memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));

    pInfo->Name = StrToWStr(CPUBrandString);

    return true;
}

bool HWInfo::GetInstructionSupport(Processor* const pInfo) {
    INT CPUInfo[4] { 0 };

    // Call default CPUID (EAX=1)
    __cpuid(CPUInfo, 1);

    // Check if SSE, SSE2 are supported in EDX register
    pInfo->SupportMMX = (CPUInfo[3] & (1 << 23)) != 0; // EDX 23th bit (MMX)
    pInfo->SupportSSE = (CPUInfo[3] & (1 << 25)) != 0; // EDX 25th bit (SSE)
    pInfo->SupportSSE2 = (CPUInfo[3] & (1 << 26)) != 0; // EDX 26th bit (SSE2)

    // Check if SSE3, AVX, etc. are supported in ECX register
    pInfo->SupportSSE3 = (CPUInfo[2] & (1 << 0)) != 0; // ECX 0th  bit (SSE3)
    pInfo->SupportSSSE3 = (CPUInfo[2] & (1 << 9)) != 0; // ECX 9th  bit (SSSE3)
    pInfo->SupportSSE4_1 = (CPUInfo[2] & (1 << 19)) != 0; // ECX 19th bit (SSE4.1)
    pInfo->SupportSSE4_2 = (CPUInfo[2] & (1 << 20)) != 0; // ECX 20th bit (SSE4.2)
    pInfo->SupportAVX = (CPUInfo[2] & (1 << 28)) != 0; // ECX 28th bit (AVX)

    // Call CPUID (EAX=7, ECX=0)
    __cpuidex(CPUInfo, 7, 0);

    pInfo->SupportAVX2 = (CPUInfo[1] & (1 << 5)) != 0; // EBX 5th  bit (AVX2)
    pInfo->SupportAVX512F = (CPUInfo[1] & (1 << 16)) != 0; // EBX 16th bit (AVX512F)
    pInfo->SupportAVX512DQ = (CPUInfo[1] & (1 << 17)) != 0; // EBX 17th bit (AVX512DQ)
    pInfo->SupportAVX512BW = (CPUInfo[1] & (1 << 30)) != 0; // EBX 30th bit (AVX512BW)

    return true;
}

bool HWInfo::GetCoreInfo(Processor* const pInfo) {
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

    if (length == 0) 
        ReturnFalse(STR_FAIL_GET_BUFF_SIZE);

    std::vector<uint8_t> buffer(length);
    if (!GetLogicalProcessorInformationEx(
        RelationProcessorCore
        , reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data())
        , &length))
        ReturnFalse(STR_FAIL_GET_PROC_INFO);

    pInfo->Physical = 0;
    pInfo->Logical = 0;

    for (size_t offset = 0; offset < length;) {
        auto* const infoEX = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);

        if (infoEX->Relationship == RelationProcessorCore) {
            ++pInfo->Physical;
            pInfo->Logical += __popcnt64(infoEX->Processor.GroupMask->Mask);
        }

        offset += infoEX->Size;
    }

    return true;
}

bool HWInfo::GetSystemMemoryInfo(Processor* const pInfo) {
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&memInfo)) 
        ReturnFalse(STR_FAIL_GET_MEM_STAT);

    UINT64 denom = 1024 * 1024;
    pInfo->TotalPhysicalMemory = memInfo.ullTotalPhys / denom;
    pInfo->AvailablePhysicalMemory = memInfo.ullAvailPhys / denom;
    pInfo->TotalVirtualMemory = memInfo.ullTotalVirtual / denom;
    pInfo->AvailableVirtualMemory = memInfo.ullAvailVirtual / denom;

    return true;
}

void SaveWString(FILE* pFile, const std::wstring& string) {
    int Len = static_cast<int>(string.length());
    fwrite(&Len, sizeof(int), 1, pFile);
    fwrite(string.data(), sizeof(wchar_t), Len, pFile);
}

std::wstring LoadWString(FILE* pFile) {
    int Len{};
    fread(&Len, sizeof(int), 1, pFile);

    wchar_t buff[255]{};
    fread(buff, sizeof(wchar_t), Len, pFile);

    return buff;
}

void SaveAssetRef(FILE* pFile, Asset* pAsset) {
    // Asset 이 Null 인지 아닌지 저장
    bool IsNull = pAsset;
    fwrite(&IsNull, sizeof(bool), 1, pFile);

    // Asset 의 Key, RelativePath 저장
    if (nullptr != pAsset) {
        SaveWString(pFile, pAsset->GetKey());
        SaveWString(pFile, pAsset->GetRelativePath());
    }
}