#include "pch.h"

#include "Engine.hpp"

namespace {
	void CreateDebuggingConsole() {
		AllocConsole();

		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);

		SetConsoleOutputCP(CP_UTF8);

		ConsoleLog("Debugging Console Initialized");
	}

	void DestroyDebuggingConsole(BOOL bNeedToPause = FALSE) {
		ConsoleLog("Debugging Console will be terminated");
		if (bNeedToPause) system("pause");
		FreeConsole();
	}
}

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow) {	
	//_CrtSetBreakAlloc(166);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

#ifdef _DEBUG
	CreateDebuggingConsole();
#endif

	std::unique_ptr<LogFile> logFile = std::make_unique<LogFile>();
	if (!Logger::Initialize(logFile.get(), L"./log.txt")) return -1;

	auto pLogFile = logFile.get();

	try {
		CheckReturn(pLogFile, Engine::GetInstance()->Initialize(
			pLogFile, hInstance, 1600, 900));

		CheckReturn(pLogFile, Engine::GetInstance()->Run());

		Engine::GetInstance()->CleanUp();
	}
	catch (const std::exception& e) {
		Logln(pLogFile, e.what());
	}

#ifdef _DEBUG
	DestroyDebuggingConsole();
#endif

	Logln(pLogFile, "Exiting application");

	return 0;
}