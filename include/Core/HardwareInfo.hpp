#pragma once

struct ProcessorInfo {
	std::string Name;

	bool SupportMMX			= false;
	bool SupportSSE			= false;
	bool SupportSSE2		= false;
	bool SupportSSE3		= false;
	bool SupportSSSE3		= false;
	bool SupportSSE4_1		= false;
	bool SupportSSE4_2		= false;
	bool SupportAVX			= false;
	bool SupportAVX2		= false;
	bool SupportAVX512F		= false; // AVX-512 Foundation
	bool SupportAVX512DQ	= false; // AVX-512 Doubleword & Quadword
	bool SupportAVX512BW	= false; // AVX-512 Byte & Word

	UINT64 Physical	= 0;
	UINT64 Logical	= 0;

	UINT64 TotalPhysicalMemory		= 0;
	UINT64 AvailablePhysicalMemory	= 0;
	UINT64 TotalVirtualMemory		= 0;
	UINT64 AvailableVirtualMemory	= 0;
};

class HardwareInfo {
public:
	static bool GetProcessorInfo(ProcessorInfo& info);

public:
	static bool GetProcessorName(ProcessorInfo& info);
	static bool GetInstructionSupport(ProcessorInfo& info);
	static bool GetCoreInfo(ProcessorInfo& info);
	static bool GetSystemMemoryInfo(ProcessorInfo& info);
};