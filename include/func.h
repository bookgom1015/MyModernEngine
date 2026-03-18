#pragma once

#include <string>
#include <format>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

inline std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return std::wstring();

	int size = MultiByteToWideChar(
		CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring result(size, 0);

	MultiByteToWideChar(
		CP_UTF8, 0, str.data(), (int)str.size(), &result[0], size);

	return result;
}
#ifndef StrToWStr
#define StrToWStr(__x) StringToWString(__x)
#endif // StrToWStr

inline std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();

	int size = WideCharToMultiByte(
		CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(size, 0);

	WideCharToMultiByte(
		CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size, nullptr, nullptr);

	return result;
}
#ifndef WStrToStr
#define WStrToStr(__x) WStringToString(__x)
#endif // WStrToStr

struct LogFile;

class Logger {
public:
	static BOOL Initialize(LPCWSTR filePath);

	static void LogFn(const std::string& msg);
	static void LogFn(const std::wstring& msg);

	static BOOL SetTextToWnd(HWND hWnd, LPCWSTR text);
	static BOOL AppendTextToWnd(HWND hWnd, LPCWSTR text);
private:
	static LogFile mLogFile;
};

#ifndef Log
#define Log(__x, ...) {									\
	std::vector<std::string> _msgs{ __x, __VA_ARGS__ };	\
	std::stringstream _sstream{};						\
	for (const auto& _msg : _msgs) _sstream << _msg;	\
	Logger::LogFn(_sstream.str());						\
}
#endif // Log

#ifndef Logln
#define Logln(__x, ...) {								\
	std::vector<std::string> _msgs{ __x, __VA_ARGS__ };	\
	std::stringstream _sstream{};						\
	for (const auto& _msg : _msgs) _sstream << _msg;	\
	_sstream << '\n';									\
	Logger::LogFn(_sstream.str());						\
}
#endif // Logln

#ifndef WLog
#define WLog(__x, ...) {									\
	std::vector<std::wstring> _msgs{ __x, __VA_ARGS__ };	\
	std::wstringstream _wsstream{};							\
	for (const auto& _msg : _msgs) _wsstream << _msg;		\
	Logger::LogFn(_wsstream.str());							\
}
#endif // WLog

#ifndef WLogln
#define WLogln(__x, ...) {									\
	std::vector<std::wstring> _msgs{ __x, __VA_ARGS__ };	\
	std::wstringstream _wsstream{};							\
	for (const auto& _msg : _msgs) _wsstream << _msg;		\
	_wsstream << L'\n';										\
	Logger::LogFn(_wsstream.str());							\
}
#endif // WLogln

#ifndef ConsoleLog
#define ConsoleLog(__x, ...) {								\
	std::vector<std::string> _msgs = { __x, __VA_ARGS__ };	\
	std::stringstream _sstream;								\
	for (const auto& _msg : _msgs) _sstream << _msg;		\
	std::cout << _sstream.str() << std::endl;				\
}
#endif // ConsoleLog

#ifndef ConsoleErr
#define ConsoleErr(__x, ...) {								\
	std::vector<std::string> _msgs = { __x, __VA_ARGS__ };	\
	std::stringstream _sstream;								\
	for (const auto& _msg : _msgs) _sstream << _msg;		\
	std::err << _sstream.str() << std::endl;				\
}
#endif // ConsoleErr

#ifndef CheckReturn
#define CheckReturn(__statement) {							\
	try {													\
		const bool _result = __statement;					\
		if (!_result) {										\
			auto _msg = std::format("[Error] {}; {}; \n"	\
				, __FILE__, __LINE__);						\
			Logger::LogFn(_msg);							\
			return false;									\
		}													\
	}														\
	catch (const std::exception& e) {						\
		auto _msg = std::format("Exception {}; {}; {} \n"	\
			, __FILE__, __LINE__, e.what());				\
		Logger::LogFn(_msg);								\
		return false;										\
	}														\
}
#endif // CheckReturn

#ifndef CheckHResult
#define CheckHResult(__statement) {											\
	try {																	\
		const HRESULT _result = __statement;								\
		if (FAILED(_result)) {												\
			auto _msg = std::format("[Error] {}; {}; HRESULT: 0x{:X} \n"	\
				, __FILE__, __LINE__, static_cast<std::uint32_t>(_result));	\
			Logger::LogFn(_msg);											\
			return false;													\
		}																	\
	}																		\
	catch (const std::exception& e) {										\
		auto _msg = std::format("Exception {}; {}; {} \n"					\
			, __FILE__, __LINE__, e.what());								\
		Logger::LogFn(_msg);												\
		return false;														\
	}																		\
}
#endif // CheckHResult

#ifndef ReturnFalse
#define ReturnFalse(__msg) {						\
	auto _msg = std::format("[Error] {}; {}; {} \n"	\
		, __FILE__, __LINE__, __msg);				\
	Logger::LogFn(_msg);							\
	return false;									\
}
#endif // ReturnFalse

struct Processor;

class HWInfo {
public:
	static bool ProcessorInfo(Processor* const pInfo);

public:
	static bool GetProcessorName(Processor* const pInfo);
	static bool GetInstructionSupport(Processor* const pInfo);
	static bool GetCoreInfo(Processor* const pInfo);
	static bool GetSystemMemoryInfo(Processor* const pInfo);
};

void SaveWString(FILE* pFile, const std::wstring& string);

std::wstring LoadWString(FILE* pFile);

void SaveAssetRef(FILE* _File, class Asset* _Asset);