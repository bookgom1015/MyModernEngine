#pragma once

#include <mutex>
#include <Windows.h>

struct LogFile {
	LogFile() : Handle(INVALID_HANDLE_VALUE) {}

	~LogFile() {
		if (Handle != INVALID_HANDLE_VALUE && Handle != NULL) {
			CloseHandle(Handle);
			Handle = INVALID_HANDLE_VALUE;
		}
	}

	// Non-copyable to avoid accidental double-close of the handle.
	LogFile(const LogFile&) = delete;
	LogFile& operator=(const LogFile&) = delete;

	HANDLE Handle;
	std::mutex Mutex;
};

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