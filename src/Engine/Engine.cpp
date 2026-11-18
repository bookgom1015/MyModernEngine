#include "Engine/pch.h"
#include "Engine/Engine.hpp"

#include "Engine/PathManager.hpp"
#include "Engine/InputManager.hpp"
#include "Engine/TimeManager.hpp"
#include "Engine/AssetManager.hpp"
#include "Engine/LevelManager.hpp"
#include "Engine/EditorManager.hpp"
#include "Engine/Profiler.hpp"
#include "Engine/TaskManager.hpp"
#include "Engine/PhysicsManager.hpp"
#include "Engine/LightManager.hpp"
#include "Engine/AudioManager.hpp"
#include "Engine/CCamera.hpp"

#include RENDERER_HEADER

Engine::Engine() 
	: mhInst{}
	, mhMainWnd{}
	, mResolution{}
	, mbAppPaused{}
	, mbMinimized{}
	, mbMaximized{}
	, mbResizing{}
	, mbFullscreenState{}
	, mProcessorInfo{} {}

Engine::~Engine() {}

bool Engine::Initialize(
	HINSTANCE hInst
	, unsigned width
	, unsigned height) {
	mhInst = hInst;
	mResolution = Uint2(width, height);

	CheckReturn(InitializeWindow());

	HardwareInfo::GetProcessorInfo(mProcessorInfo);

#ifdef _DEBUG
	const auto& YesOrNo = [](bool state) {
		return state ? "YES" : "NO";
	};

	Logln("--------------------------------------------------------------------");
	Logln("Processor: ", mProcessorInfo.Name.c_str());
	Logln("Instruction support:");
	Logln("    MMX: ", YesOrNo(mProcessorInfo.SupportMMX));
	Logln("    SSE: ", YesOrNo(mProcessorInfo.SupportSSE));
	Logln("    SSE2: ", YesOrNo(mProcessorInfo.SupportSSE2));
	Logln("    SSE3: ", YesOrNo(mProcessorInfo.SupportSSE3));
	Logln("    SSSE3: ", YesOrNo(mProcessorInfo.SupportSSSE3));
	Logln("    SSE4.1: ", YesOrNo(mProcessorInfo.SupportSSE4_1));
	Logln("    SSE4.2: ", YesOrNo(mProcessorInfo.SupportSSE4_2));
	Logln("    AVX: ", YesOrNo(mProcessorInfo.SupportAVX));
	Logln("    AVX2: ", YesOrNo(mProcessorInfo.SupportAVX2));
	Logln("    AVX512: ", YesOrNo(mProcessorInfo.SupportAVX512F && mProcessorInfo.SupportAVX512DQ && mProcessorInfo.SupportAVX512BW));
	Logln("Physical core count: ", std::to_string(mProcessorInfo.Physical));
	Logln("Logical core count: ", std::to_string(mProcessorInfo.Logical));
	Logln("Total physical memory: ", std::to_string(mProcessorInfo.TotalPhysicalMemory), "MB");
	Logln("Total virtual memory: ", std::to_string(mProcessorInfo.TotalVirtualMemory), "MB");
	Logln("--------------------------------------------------------------------");
#endif
	
	CheckReturn(PATH_MANAGER->Initialize());
	CheckReturn(INPUT_MANAGER->Initialize());
	CheckReturn(TIME_MANAGER->Initialize());
	CheckReturn(PHYSICS_MANAGER->Initialize());
	CheckReturn(LIGHT_MANAGER->Initialize());
	CheckReturn(AUDIO_MANAGER->Initialize());

	RENDERER->SetShaderDirectory(std::format(L"{}Shader\\D3D12\\", CONTENT_PATH));
	RENDERER->SetEditorCallbacks(
		[](const std::string& name, std::uint64_t id) { EDITOR_MANAGER->AddDisplayTexture(name, static_cast<ImTextureID>(id)); },
		[]() {
			return Profiler::Measure(Profiler::Stage::EditorDraw, [] { return EDITOR_MANAGER->Draw(); });
		});
	CheckReturn(RENDERER->Initialize(mhMainWnd, width, height));
	CheckReturn(EDITOR_MANAGER->Initialize());

	CheckReturn(ASSET_MANAGER->Initialize());
	CheckReturn(LEVEL_MANAGER->Initialize());

	return true;
}

void Engine::CleanUp() {
	RENDERER->CleanUp();
	EDITOR_MANAGER->CleanUp();
}

bool Engine::Run() {
	CheckReturn(BeforeBegin());

	MSG msg{};
	PerformanceTimer frameTimer;
	bool hasPreviousFrame = false;

	while (true) {
		// 창 메시지가 있으면 처리합니다.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// 대기 중인 메시지가 없으면 애니메이션과 게임 로직을 갱신합니다.
		else {
			// 이전 프레임의 작업 시간과 프레임 시작 간격을 함께 기록합니다.
			if (hasPreviousFrame) {
				frameTimer.Stop();
				Profiler::RecordFrame(static_cast<float>(frameTimer.ElapsedMilliseconds()));
			}
			frameTimer.Start();
			hasPreviousFrame = true;

			CheckReturn(TIME_MANAGER->Update());

			CheckReturn(Profiler::Measure(Profiler::Stage::Input, [this] { return Input(); }));
			CheckReturn(Update());
			CheckReturn(Profiler::Measure(Profiler::Stage::RendererDraw, [this] { return Draw(); }));

			CheckReturn(Profiler::Measure(Profiler::Stage::Tasks, [] { return TASK_MANAGER->Update(); }));
		}
	}

	return true;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// CreateWindow가 반환되기 전에 WM_CREATE 등의 메시지를 받을 수 있으므로
	// mhMainWnd가 유효해지기 전에도 메시지를 처리할 수 있도록 hwnd를 전달합니다.
	return Engine::GetInstance()->MsgProc(hWnd, msg, wParam, lParam);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Engine::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
		// 창이 활성화되거나 비활성화되면 WM_ACTIVATE 메시지가 전달됩니다.
		// 창이 비활성화되면 게임을 일시 정지하고
		// 창이 다시 활성화되면 게임을 재개합니다.
	case WM_ACTIVATE: {
		//BOOL sleepable = mPowerManager->IsSleepable();

		if (LOWORD(wParam) == WA_INACTIVE) {
			//if (!sleepable) mPowerManager->SetMode(FALSE);

			mbAppPaused = TRUE;
		}
		else {
			//if (sleepable) mPowerManager->SetMode(TRUE);

			mbAppPaused = FALSE;
		}
		return 0;
	}
	// 사용자가 창 크기를 조절하면 WM_SIZE 메시지가 전달됩니다.
	case WM_SIZE: {
		// 새 클라이언트 영역의 크기를 저장합니다.
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);

		//BOOL sleepable = mPowerManager->IsSleepable();
		if (wParam == SIZE_MINIMIZED) {
			//if (!sleepable) mPowerManager->SetMode(FALSE);

			mbAppPaused = TRUE;
			mbMinimized = TRUE;
			mbMaximized = FALSE;
		}
		else if (wParam == SIZE_MAXIMIZED) {
			//if (sleepable) mPowerManager->SetMode(FALSE);

			mbAppPaused = FALSE;
			mbMinimized = FALSE;
			mbMaximized = TRUE;
			OnResize(width, height);
		}
		else if (wParam == SIZE_RESTORED) {
			// 최소화 상태에서 복원하는 경우입니다.
			if (mbMinimized) {
				//if (sleepable) mPowerManager->SetMode(FALSE);

				mbAppPaused = FALSE;
				mbMinimized = FALSE;
				OnResize(width, height);
			}

			// 최대화 상태에서 복원하는 경우입니다.
			else if (mbMaximized) {
				//if (sleepable) mPowerManager->SetMode(FALSE);

				mbAppPaused = FALSE;
				mbMaximized = FALSE;
				OnResize(width, height);
			}
			// 사용자가 창 테두리를 드래그하는 동안에는
			// WM_SIZE 메시지가 연속으로 전달됩니다.
			// 메시지마다 버퍼 크기를 바꾸면 불필요한 작업으로 성능이 저하됩니다.
			// 따라서 드래그 중에는 버퍼 크기 조정을 미룹니다.
			// 사용자가 창 크기 조절을 마치고
			// 테두리를 놓으면
			// WM_EXITSIZEMOVE 메시지가 전달되며
			// 이때 버퍼를 재설정합니다.
			else if (mbResizing) {

			}
			// SetWindowPos 또는 mSwapChain->SetFullscreenState 같은 API 호출로 발생합니다.
			else {
				OnResize(width, height);
			}
		}
		return 0;
	}
	// 테두리 없는 창의 크기를 조절할 수 있도록 비클라이언트 영역의 히트 테스트를 처리합니다.
	case WM_NCHITTEST: {
		const int border = 8;

		RECT rc;
		GetClientRect(mhMainWnd, &rc);

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(mhMainWnd, &pt);

		// 모서리 영역입니다.
		if (pt.x < border && pt.y < border) return HTTOPLEFT;
		if (pt.x > rc.right - border && pt.y < border) return HTTOPRIGHT;
		if (pt.x < border && pt.y > rc.bottom - border) return HTBOTTOMLEFT;
		if (pt.x > rc.right - border && pt.y > rc.bottom - border) return HTBOTTOMRIGHT;

		// 왼쪽과 오른쪽 영역입니다.
		if (pt.x < border) return HTLEFT;
		if (pt.x > rc.right - border) return HTRIGHT;

		// 위쪽과 아래쪽 영역입니다.
		if (pt.y < border) return HTTOP;
		if (pt.y > rc.bottom - border) return HTBOTTOM;

		return HTCLIENT;
	}
	// 창의 크기 조절을 시작하면 WM_ENTERSIZEMOVE 메시지가 전달됩니다.
	case WM_ENTERSIZEMOVE: {
		mbAppPaused = TRUE;
		mbResizing = TRUE;
		return 0;
	}
	// 창의 크기 조절을 마치면 WM_EXITSIZEMOVE 메시지가 전달됩니다.
	// 새 창 크기에 맞춰 관련 리소스를 재설정합니다.
	case WM_EXITSIZEMOVE: {
		mbAppPaused = FALSE;
		mbResizing = FALSE;

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);

		UINT width = static_cast<UINT>(clientRect.right - clientRect.left);
		UINT height = static_cast<UINT>(clientRect.bottom - clientRect.top);

		OnResize(width, height);
		return 0;
	}
	// 창을 파괴할 때 WM_DESTROY 메시지가 전달됩니다.
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	// 메뉴가 활성화된 상태에서 사용자가 메뉴 접근 키나
	// 단축키로 등록되지 않은 키를 누르면 WM_MENUCHAR 메시지가 전달됩니다.
	case WM_MENUCHAR: {
		// Alt+Enter를 눌렀을 때 경고음이 나지 않게 합니다.
		return MAKELRESULT(0, MNC_CLOSE);
	}
	// 창이 너무 작아지지 않도록 이 메시지를 처리합니다.
	case WM_GETMINMAXINFO: {
		return 0;
	}
	case WM_KEYUP:
	case WM_KEYDOWN: {
		
		return 0;
	}
	case WM_LBUTTONDOWN: {
		return 0;
	}
	case WM_MOUSEMOVE: {
		
		return 0;
	}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool Engine::InitializeWindow() {
	WNDCLASS wc{};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MyModernGameEngine";
	if (!RegisterClassW(&wc)) 
		ReturnFalse("Failed to register the window class");

	// 요청한 클라이언트 영역 크기를 기준으로 창의 사각형 크기를 계산합니다.
	RECT R = { 0, 0, static_cast<LONG>(mResolution.x), static_cast<LONG>(mResolution.y) };

	// WS_OVERLAPPEDWINDOW를 사용한다면 AdjustWindowRect가 필요합니다.
	// 여기서 사용하는 WS_POPUP에는 테두리와 제목 표시줄이 없으므로
	// 클라이언트 영역과 창 전체 영역의 크기가 같습니다.
	
	//if (!AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE)) 
	//	ReturnFalse(mpLogFile, "Failed to get window rect");

	const INT width = R.right - R.left;
	const INT height = R.bottom - R.top;

	const INT outputWidth = GetSystemMetrics(SM_CXSCREEN);
	const INT outputHeight = GetSystemMetrics(SM_CYSCREEN);

	const INT clientPosX = static_cast<INT>((outputWidth - mResolution.x) * 0.5f);
	const INT clientPosY = static_cast<INT>((outputHeight - mResolution.y) * 0.5f);

	mhMainWnd = CreateWindowW(
		L"MyModernGameEngine", L"MyModernGameEngine",
		WS_POPUP,
		clientPosX, clientPosY,
		width, height,
		0, 0, mhInst, 0);
	if (!mhMainWnd) ReturnFalse("Failed to create the window");

	ShowWindow(mhMainWnd, SW_SHOW);
	if (!UpdateWindow(mhMainWnd)) ReturnFalse("Failed to update window");

	return true;
}

bool Engine::OnResize(unsigned width, unsigned height) {
	if (mResolution.x == width && mResolution.y == height) return true;
	mResolution = Uint2(width, height);

	CheckReturn(RENDERER->OnResize(width, height));

	return true;
}

bool Engine::Input() {
	CheckReturn(INPUT_MANAGER->Update());

	return true;
}

bool Engine::Update() {
	CheckReturn(Profiler::Measure(Profiler::Stage::Assets, [] { return ASSET_MANAGER->Update(); }));
	CheckReturn(Profiler::Measure(Profiler::Stage::Level, [] { return LEVEL_MANAGER->Update(); }));
	CheckReturn(Profiler::Measure(Profiler::Stage::EditorUpdate, [] { return EDITOR_MANAGER->Update(); }));

	RENDERER->ClearRenderObjects();

	CCamera* camera = nullptr;
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing) {
		if (auto level = LEVEL_MANAGER->GetCurrentLevel(); level != nullptr) {
			// 이전 등록 순서를 유지합니다: 마지막 레벨 카메라가 활성화됩니다.
			for (UINT i = 0; i < MAX_LAYER; ++i) {
				for (const auto& object : level->GetLayer(i)->GetAllObjects()) {
					if (auto candidate = object->Camera(); 
						candidate != nullptr) camera = candidate.Get();
				}
			}
		}
	}
	else {
		camera = EDITOR_MANAGER->GetEditorCamera();
	}

	if (camera != nullptr) 
		camera->SubmitRenderData();

	std::vector<const LightData*> lightSources;
	LIGHT_MANAGER->GetLightData(lightSources);

	std::vector<LightData> lights;
	lights.reserve(lightSources.size());

	for (const auto light : lightSources) 
		lights.push_back(*light);

	RENDERER->UpdateLights(lights);
	CheckReturn(Profiler::Measure(Profiler::Stage::RendererUpdate, [] { return RENDERER->Update(E_DT); }));

	return true;
}

bool Engine::Draw() {
	CheckReturn(RENDERER->Draw());

	return true;
}

bool Engine::BeforeBegin() {
	CheckReturn(EDITOR_MANAGER->SetSystemSounds());

	return true;
}