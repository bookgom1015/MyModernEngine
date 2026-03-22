#pragma once

struct Processor {
	std::wstring Name;

	BOOL SupportMMX = FALSE;
	BOOL SupportSSE = FALSE;
	BOOL SupportSSE2 = FALSE;
	BOOL SupportSSE3 = FALSE;
	BOOL SupportSSSE3 = FALSE;
	BOOL SupportSSE4_1 = FALSE;
	BOOL SupportSSE4_2 = FALSE;
	BOOL SupportAVX = FALSE;
	BOOL SupportAVX2 = FALSE;
	BOOL SupportAVX512F = FALSE;  // AVX-512 Foundation
	BOOL SupportAVX512DQ = FALSE; // AVX-512 Doubleword & Quadword
	BOOL SupportAVX512BW = FALSE; // AVX-512 Byte & Word

	UINT64 Physical = 0;
	UINT64 Logical = 0;

	UINT64 TotalPhysicalMemory = 0;
	UINT64 AvailablePhysicalMemory = 0;
	UINT64 TotalVirtualMemory = 0;
	UINT64 AvailableVirtualMemory = 0;
};

struct TaskInfo {
	ETask::Type	Type;
	DWORD_PTR	Param_0;
	DWORD_PTR	Param_1;
	DWORD_PTR	Param_2;
};

struct ScriptInfo {
	std::string Name;
	ScriptFactory Factory;
};