#include "func.h"

#include "struct.h"

#include <mutex>
#include <vector>

namespace {
    const char* const STR_FAIL_GET_PROC_INFO = "Failed to retrieve processor information.";
    const char* const STR_FAIL_GET_BUFF_SIZE = "Failed to get required buffer size.";
    const char* const STR_FAIL_GET_MEM_STAT = "Failed to get memory status";
}

std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return std::wstring();

	int size = MultiByteToWideChar(
		CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring result(size, 0);

	MultiByteToWideChar(
		CP_UTF8, 0, str.data(), (int)str.size(), &result[0], size);

	return result;
}

std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();

	int size = WideCharToMultiByte(
		CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(size, 0);

	WideCharToMultiByte(
		CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size, nullptr, nullptr);

	return result;
}

BOOL Logger::Initialize(LogFile* const pLogFile, LPCWSTR filePath) {
	pLogFile->Handle = CreateFile(
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

	return WriteFile(pLogFile->Handle, &bom, 2, &writtenBytes, NULL);
}

void Logger::LogFn(LogFile* const pLogFile, const std::string& msg) {
	std::wstring wstr = StrToWStr(msg);

	DWORD writtenBytes = 0;
	{
		std::lock_guard<std::mutex> lock(pLogFile->Mutex);

		WriteFile(
			pLogFile->Handle,
			wstr.c_str(),
			static_cast<DWORD>(wstr.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
}

void Logger::LogFn(LogFile* const pLogFile, const std::wstring& msg) {
	DWORD writtenBytes = 0;
	{
		std::lock_guard<std::mutex> lock(pLogFile->Mutex);

		WriteFile(
			pLogFile->Handle,
			msg.c_str(),
			static_cast<DWORD>(msg.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
}

BOOL Logger::SetTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR text) {
	return SetWindowTextW(hWnd, text);
}

BOOL Logger::AppendTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR text) {
	const INT length = GetWindowTextLengthW(hWnd) + lstrlenW(text) + 1;

	std::vector<WCHAR> buffer(length);

	GetWindowTextW(hWnd, buffer.data(), length);
	wcscat_s(buffer.data(), length, text);
	SetWindowTextW(hWnd, buffer.data());

	return TRUE;
}

bool HWInfo::ProcessorInfo(LogFile* const pLogFile, Processor* const pInfo) {
    CheckReturn(pLogFile, GetProcessorName(pLogFile, pInfo));
    CheckReturn(pLogFile, GetInstructionSupport(pLogFile, pInfo));
    CheckReturn(pLogFile, GetCoreInfo(pLogFile, pInfo));
    CheckReturn(pLogFile, GetSystemMemoryInfo(pLogFile, pInfo));

    return true;
}

bool HWInfo::GetProcessorName(LogFile* const pLogFile, Processor* const pInfo) {
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

bool HWInfo::GetInstructionSupport(LogFile* const pLogFile, Processor* const pInfo) {
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

bool HWInfo::GetCoreInfo(LogFile* const pLogFile, Processor* const pInfo) {
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

    if (length == 0) ReturnFalse(pLogFile, STR_FAIL_GET_BUFF_SIZE);

    std::vector<uint8_t> buffer(length);
    if (!GetLogicalProcessorInformationEx(
        RelationProcessorCore
        , reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data())
        , &length))
        ReturnFalse(pLogFile, STR_FAIL_GET_PROC_INFO);

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

bool HWInfo::GetSystemMemoryInfo(LogFile* const pLogFile, Processor* const pInfo) {
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&memInfo)) ReturnFalse(pLogFile, STR_FAIL_GET_MEM_STAT);

    UINT64 denom = 1024 * 1024;
    pInfo->TotalPhysicalMemory = memInfo.ullTotalPhys / denom;
    pInfo->AvailablePhysicalMemory = memInfo.ullAvailPhys / denom;
    pInfo->TotalVirtualMemory = memInfo.ullTotalVirtual / denom;
    pInfo->AvailableVirtualMemory = memInfo.ullAvailVirtual / denom;

    return true;
}