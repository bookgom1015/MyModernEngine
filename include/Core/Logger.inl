#ifndef __LOGGER_INL__
#define __LOGGER_INL__

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

#ifndef Log
#define Log(fmt, ...) do {										\
    Logger::LogFn(std::format(fmt __VA_OPT__(,) __VA_ARGS__));	\
} while (false)
#endif // Log

#ifndef Logln
#define Logln(fmt, ...) do {									\
    Logger::LogFn(												\
        std::format(fmt __VA_OPT__(,) __VA_ARGS__) + '\n');		\
} while (false)
#endif // Logln

#ifndef WLog
#define WLog(fmt, ...) do {										\
    Logger::LogFn(std::format(fmt __VA_OPT__(,) __VA_ARGS__));	\
} while (false)
#endif // WLog

#ifndef WLogln
#define WLogln(fmt, ...) do {									\
    Logger::LogFn(												\
        std::format(fmt __VA_OPT__(,) __VA_ARGS__) + L'\n');	\
} while (false)
#endif // WLogln

#ifndef ConsoleLog
#define ConsoleLog(fmt, ...) do {						\
    std::cout											\
        << std::format(fmt __VA_OPT__(,) __VA_ARGS__)	\
        << std::endl;									\
} while (false)
#endif // ConsoleLog

#ifndef ConsoleErr
#define ConsoleErr(fmt, ...) do {						\
    std::cerr											\
        << std::format(fmt __VA_OPT__(,) __VA_ARGS__)	\
        << std::endl;									\
} while (false)
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
		auto _msg = std::format("[Exception] {}; {}; {} \n"	\
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
		auto _msg = std::format("[Exception] {}; {}; {} \n"					\
			, __FILE__, __LINE__, e.what());								\
		Logger::LogFn(_msg);												\
		return false;														\
	}																		\
}
#endif // CheckHResult

#ifndef ReturnFalse
#define ReturnFalse(__msg, ...) do {								\
    auto _format = std::format(__msg __VA_OPT__(,) __VA_ARGS__);	\
    auto _msg = std::format("[Error] {}; {}; {}\n",                 \
        __FILE__, __LINE__, _format);								\
    Logger::LogFn(_msg);											\
    return false;													\
} while (false)
// do { ... } while (false) 를 사용하여 매크로가 단일 문장처럼 동작하도록 함
#endif // ReturnFalse

#ifndef NullCheck
#define NullCheck(__object) {													\
	if (__object == nullptr) {													\
		auto _msg = std::format(												\
			"[Error] Null check failed; {}; line: {} \n", __FILE__, __LINE__);	\
		Logger::LogFn(_msg);													\
		return false;															\
	}																			\
}
#endif // NullCheck

#endif // __LOGGER_INL__