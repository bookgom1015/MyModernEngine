#include "pch.h"

#include "Engine.hpp"

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow) {	
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

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

	return 0;
}