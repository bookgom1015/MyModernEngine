#include "Core/pch.h"

LogFile Logger::mLogFile{};

LogFile::LogFile() 
	: Handle{ INVALID_HANDLE_VALUE }, Mutex{} {}

LogFile::~LogFile() {
	if (Handle != INVALID_HANDLE_VALUE && Handle != NULL) {
		CloseHandle(Handle);
		Handle = INVALID_HANDLE_VALUE;
	}
}

bool Logger::Initialize(LPCWSTR filePath) {
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

bool Logger::SetTextToWnd(HWND hWnd, LPCWSTR text) {
	return SetWindowTextW(hWnd, text);
}

bool Logger::AppendTextToWnd(HWND hWnd, LPCWSTR text) {
	const INT length = GetWindowTextLengthW(hWnd) + lstrlenW(text) + 1;

	std::vector<WCHAR> buffer(length);

	GetWindowTextW(hWnd, buffer.data(), length);
	wcscat_s(buffer.data(), length, text);
	SetWindowTextW(hWnd, buffer.data());

	return TRUE;
}