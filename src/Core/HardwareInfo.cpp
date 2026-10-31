#include "Core/pch.h"
#include "Core/HardwareInfo.hpp"

namespace {
    const char* const STR_FAIL_GET_PROC_INFO = "Failed to retrieve processor information.";
    const char* const STR_FAIL_GET_BUFF_SIZE = "Failed to get required buffer size.";
    const char* const STR_FAIL_GET_MEM_STAT = "Failed to get memory status";
}

bool HardwareInfo::GetProcessorInfo(ProcessorInfo& info) {
    CheckReturn(GetProcessorName(info));
    CheckReturn(GetInstructionSupport(info));
    CheckReturn(GetCoreInfo(info));
    CheckReturn(GetSystemMemoryInfo(info));

    return true;
}

bool HardwareInfo::GetProcessorName(ProcessorInfo& info) {
    INT CPUInfo[4] = { -1 };
    CHAR CPUBrandString[0x40] = { 0 };

	// 0x80000002는 프로세서 이름의 첫 번째 부분을 가져오는 데 사용됩니다.
    __cpuid(CPUInfo, 0x80000002);
    memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));

	// 0x80000003 와 0x80000004는 프로세서 이름의 나머지 부분을 가져오는 데 사용됩니다.
    __cpuid(CPUInfo, 0x80000003);
    memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));

	// 0x80000004는 프로세서 이름의 마지막 부분을 가져오는 데 사용됩니다.
    __cpuid(CPUInfo, 0x80000004);
    memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));

    std::stringstream ssstream;
    ssstream << CPUBrandString;

    info.Name = ssstream.str();

    return true;
}

bool HardwareInfo::GetInstructionSupport(ProcessorInfo& info) {
    INT CPUInfo[4] = { 0 };

	// CPUID 기능을 사용하여 CPU의 기능을 확인합니다.
	// EAX=1, ECX=0을 사용하여 CPUID를 호출합니다.
    __cpuid(CPUInfo, 1);

	// EDX 레지스터의 23번째 비트(MMX), 25번째 비트(SSE), 26번째 비트(SSE2)를 확인하여 지원 여부를 확인합니다.
    info.SupportMMX = (CPUInfo[3] & (1 << 23)) != 0; // EDX 23th bit (MMX)
    info.SupportSSE = (CPUInfo[3] & (1 << 25)) != 0; // EDX 25th bit (SSE)
    info.SupportSSE2 = (CPUInfo[3] & (1 << 26)) != 0; // EDX 26th bit (SSE2)

    // ECX 레지스터의 0번째 비트(SSE3), 9번째 비트(SSSE3), 19번째 비트(SSE4.1), 20번째 비트(SSE4.2), 28번째 비트(AVX)를 확인하여 지원 여부를 확인합니다.
    info.SupportSSE3 = (CPUInfo[2] & (1 << 0)) != 0; // ECX 0th  bit (SSE3)
    info.SupportSSSE3 = (CPUInfo[2] & (1 << 9)) != 0; // ECX 9th  bit (SSSE3)
    info.SupportSSE4_1 = (CPUInfo[2] & (1 << 19)) != 0; // ECX 19th bit (SSE4.1)
    info.SupportSSE4_2 = (CPUInfo[2] & (1 << 20)) != 0; // ECX 20th bit (SSE4.2)
    info.SupportAVX = (CPUInfo[2] & (1 << 28)) != 0; // ECX 28th bit (AVX)
        
    // EAX=7, ECX=0을 사용하여 CPUID를 호출합니다.
    __cpuidex(CPUInfo, 7, 0);

	// EBX 레지스터의 5번째 비트(AVX2), 16번째 비트(AVX512F), 17번째 비트(AVX512DQ), 30번째 비트(AVX512BW)를 확인하여 지원 여부를 확인합니다.
    info.SupportAVX2 = (CPUInfo[1] & (1 << 5)) != 0; // EBX 5th  bit (AVX2)
    info.SupportAVX512F = (CPUInfo[1] & (1 << 16)) != 0; // EBX 16th bit (AVX512F)
    info.SupportAVX512DQ = (CPUInfo[1] & (1 << 17)) != 0; // EBX 17th bit (AVX512DQ)
    info.SupportAVX512BW = (CPUInfo[1] & (1 << 30)) != 0; // EBX 30th bit (AVX512BW)

    return true;
}

bool HardwareInfo::GetCoreInfo(ProcessorInfo& info) {
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

    if (length == 0) ReturnFalse(STR_FAIL_GET_BUFF_SIZE);

    std::vector<uint8_t> buffer(length);
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data()), &length))
        ReturnFalse(STR_FAIL_GET_PROC_INFO);

    info.Physical = 0;
    info.Logical = 0;

    for (size_t offset = 0; offset < length;) {
        auto* const infoEX = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);

        if (infoEX->Relationship == RelationProcessorCore) {
            ++info.Physical;
            info.Logical += __popcnt64(infoEX->Processor.GroupMask->Mask);
        }

        offset += infoEX->Size;
    }

    return true;
}

bool HardwareInfo::GetSystemMemoryInfo(ProcessorInfo& info) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&memInfo)) ReturnFalse(STR_FAIL_GET_MEM_STAT);

    UINT64 denom = 1024 * 1024;
    info.TotalPhysicalMemory = memInfo.ullTotalPhys / denom;
    info.AvailablePhysicalMemory = memInfo.ullAvailPhys / denom;
    info.TotalVirtualMemory = memInfo.ullTotalVirtual / denom;
    info.AvailableVirtualMemory = memInfo.ullAvailVirtual / denom;

    return true;
}