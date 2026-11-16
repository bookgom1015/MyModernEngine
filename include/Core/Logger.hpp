#pragma once

inline std::wstring StringToWString(const std::string& str);
#ifndef StrToWStr
#define StrToWStr(__x) StringToWString(__x)
#endif // StrToWStr

inline std::string WStringToString(const std::wstring& wstr);
#ifndef WStrToStr
#define WStrToStr(__x) WStringToString(__x)
#endif // WStrToStr

struct LogFile {
public:
	HANDLE Handle;
	std::mutex Mutex;

public:
	LogFile();
	~LogFile();

	// 핸들을 실수로 두 번 닫는 일을 방지하기 위해 복사를 금지합니다.
	LogFile(const LogFile&) = delete;
	LogFile& operator=(const LogFile&) = delete;
};

class Logger {
public:
	static bool Initialize(LPCWSTR filePath);

	static void LogFn(const std::string& msg);
	static void LogFn(const std::wstring& msg);

	static bool SetTextToWnd(HWND hWnd, LPCWSTR text);
	static bool AppendTextToWnd(HWND hWnd, LPCWSTR text);

private:
	static LogFile mLogFile;
};

#include "Logger.inl"