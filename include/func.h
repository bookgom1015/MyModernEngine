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

	// Non-copyable to avoid accidental double-close of the handle.
	LogFile(const LogFile&) = delete;
	LogFile& operator=(const LogFile&) = delete;
};

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

bool GetProcessorInfo(Processor* const pInfo);

void SaveWString(FILE* pFile, const std::wstring& string);
std::wstring LoadWString(FILE* pFile);

void SaveAssetRef(FILE* _File, class Asset* _Asset);

// 64-bit FNV-1a
constexpr uint64_t HashString(std::string_view str);
// wchar_t 버전
constexpr uint64_t HashWString(std::wstring_view str);

namespace EAsset {
	std::string AssetTypeToString(EAsset::Type type);
}

class Component;

namespace EComponent {
	std::string ComponentTypeToString(Type type);
	Type ComponentStringToType(std::string name);

	Component* GetComponent(Type type);
}

inline Hash HashCombine(Hash seed, Hash value);

void CreateGameObject(class GameObject* obj, int layer);

void ChangeLevel(const std::wstring& name);
void ChangeNewLevel(class ALevel* pNewLevel);
void ChangeLevelState(ELevelState::Type nextState);

decltype(auto) GetTimeStamp();
std::wstring MakeUniqueName(const std::wstring& name);

bool GetFile(const std::wstring& filePath, FILE*& pFile);

#include "func.inl"