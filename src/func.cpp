#include "func.h"

#include "struct.h"

#include <mutex>
#include <vector>

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
