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

static std::wstring StringToWString(const std::string& str);
#ifndef StrToWStr
#define StrToWStr(__x) StringToWString(__x)
#endif // StrToWStr

static std::string WStringToString(const std::wstring& wstr);
#ifndef WStrToStr
#define WStrToStr(__x) WStringToString(__x)
#endif // WStrToStr

struct LogFile;

class Logger {
public:
	static BOOL Initialize(LogFile* const pLogFile, LPCWSTR filePath);

	static void LogFn(LogFile* const pLogFile, const std::string& msg);
	static void LogFn(LogFile* const pLogFile, const std::wstring& msg);

	static BOOL SetTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR text);
	static BOOL AppendTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR text);
};

#ifndef Log
#define Log(__logfile, __x, ...) {						\
	std::vector<std::string> _msgs{ __x, __VA_ARGS__ };	\
	std::stringstream _sstream{};						\
	for (const auto& _msg : _msgs) _sstream << _msg;	\
	Logger::LogFn(__logfile, _sstream.str());			\
}
#endif // Log

#ifndef Logln
#define Logln(__logfile, __x, ...) {					\
	std::vector<std::string> _msgs{ __x, __VA_ARGS__ };	\
	std::stringstream _sstream{};						\
	for (const auto& _msg : _msgs) _sstream << _msg;	\
	_sstream << '\n';									\
	Logger::LogFn(__logfile, _sstream.str());			\
}
#endif // Logln

#ifndef WLog
#define WLog(__logfile, __x, ...) {							\
	std::vector<std::wstring> _msgs{ __x, __VA_ARGS__ };	\
	std::wstringstream _wsstream{};							\
	for (const auto& _msg : _msgs) _wsstream << _msg;		\
	Logger::LogFn(__logfile, _wsstream.str());				\
}
#endif // WLog

#ifndef WLogln
#define WLogln(__logfile, __x, ...) {						\
	std::vector<std::wstring> _msgs{ __x, __VA_ARGS__ };	\
	std::wstringstream _wsstream{};							\
	for (const auto& _msg : _msgs) _wsstream << _msg;		\
	_wsstream << L'\n';										\
	Logger::LogFn(__logfile, _wsstream.str());				\
}
#endif // WLogln

#ifndef CheckReturn
#define CheckReturn(__logfile, __statement) {				\
	try {													\
		const bool _result = __statement;					\
		if (!_result) {										\
			auto _msg = std::format("[Error] {}; {}; \n"	\
				, __FILE__, __LINE__);						\
			Logger::LogFn(__logfile, _msg);					\
			return false;									\
		}													\
	}														\
	catch (const std::exception& e) {						\
		auto _msg = std::format("Exception {}; {}; {} \n"	\
			, __FILE__, __LINE__, e.what());				\
		Logger::LogFn(__logfile, _msg);						\
		return false;										\
	}														\
}
#endif // CheckReturn

#ifndef CheckHResult
#define CheckHResult(__logfile, __statement) {								\
	try {																	\
		const HRESULT _result = __statement;								\
		if (FAILED(_result)) {												\
			auto _msg = std::format("[Error] {}; {}; HRESULT: 0x{:X} \n"	\
				, __FILE__, __LINE__, _result);								\
			Logger::LogFn(__logfile, _msg);									\
			return false;													\
		}																	\
	}																		\
	catch (const std::exception& e) {										\
		auto _msg = std::format("Exception {}; {}; {} \n"					\
			, __FILE__, __LINE__, e.what());								\
		Logger::LogFn(__logfile, _msg);										\
		return false;														\
	}																		\
}
#endif // CheckHResult

#ifndef ReturnFalse
#define ReturnFalse(__logfile, __msg) {				\
	auto _msg = std::format("[Error] {}; {}; {} \n" \
		, __FILE__, __LINE__, __msg);				\
	Logger::LogFn(__logfile, _msg);					\
	return false;									\
}
#endif // ReturnFalse