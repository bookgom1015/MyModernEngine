#include "pch.h"
#include "Engine.hpp"

#include "InputManager.hpp"
#include "EditorManager.hpp"
#include "Renderer/Renderer.hpp"

#include "EditorManager.hpp"

namespace {
	typedef Renderer* (*CreateRendererFunc)();
	typedef void (*DestroyRendererFunc)(Renderer*);
}

Engine::Engine() 
	: mpLogFile{}
	, mhInst{}
	, mhMainWnd{}
	, mResolution{}
	, mbAppPaused{}
	, mbMinimized{}
	, mbMaximized{}
	, mbResizing{}
	, mbFullscreenState{}
	, mbDestroyed{}
	, mInputManager{}
	, mEditorManager{}
	, mRenderer{ nullptr,nullptr }
	, mhRendererLibModule{} 
	, mProcessor{} {}

Engine::~Engine() {}

bool Engine::Initialize(
	LogFile* const pLogFile
	, HINSTANCE hInst
	, unsigned width
	, unsigned height) {
	mpLogFile = pLogFile;
	mhInst = hInst;
	mResolution = Uint2(width, height);

	CheckReturn(mpLogFile, InitializeWindow());

	HWInfo::ProcessorInfo(mpLogFile, &mProcessor);

#ifdef _DEBUG
	const auto& YesOrNo = [](BOOL state) {
		return state ? L"YES" : L"NO";
	};

	WLogln(mpLogFile, L"--------------------------------------------------------------------");
	WLogln(mpLogFile, L"Processor: ", mProcessor.Name.c_str());
	WLogln(mpLogFile, L"Instruction support:");
	WLogln(mpLogFile, L"    MMX: ", YesOrNo(mProcessor.SupportMMX));
	WLogln(mpLogFile, L"    SSE: ", YesOrNo(mProcessor.SupportSSE));
	WLogln(mpLogFile, L"    SSE2: ", YesOrNo(mProcessor.SupportSSE2));
	WLogln(mpLogFile, L"    SSE3: ", YesOrNo(mProcessor.SupportSSE3));
	WLogln(mpLogFile, L"    SSSE3: ", YesOrNo(mProcessor.SupportSSSE3));
	WLogln(mpLogFile, L"    SSE4.1: ", YesOrNo(mProcessor.SupportSSE4_1));
	WLogln(mpLogFile, L"    SSE4.2: ", YesOrNo(mProcessor.SupportSSE4_2));
	WLogln(mpLogFile, L"    AVX: ", YesOrNo(mProcessor.SupportAVX));
	WLogln(mpLogFile, L"    AVX2: ", YesOrNo(mProcessor.SupportAVX2));
	WLogln(mpLogFile, L"    AVX512: ", YesOrNo(mProcessor.SupportAVX512F && mProcessor.SupportAVX512DQ && mProcessor.SupportAVX512BW));
	WLogln(mpLogFile, L"Physical core count: ", std::to_wstring(mProcessor.Physical));
	WLogln(mpLogFile, L"Logical core count: ", std::to_wstring(mProcessor.Logical));
	WLogln(mpLogFile, L"Total physical memory: ", std::to_wstring(mProcessor.TotalPhysicalMemory), L"MB");
	WLogln(mpLogFile, L"Total virtual memory: ", std::to_wstring(mProcessor.TotalVirtualMemory), L"MB");
	WLogln(mpLogFile, L"--------------------------------------------------------------------");
#endif

	mInputManager = std::make_unique<InputManager>();
	CheckReturn(mpLogFile, mInputManager->Initialize());
	
	mEditorManager = std::make_unique<EditorManager>();
	CheckReturn(mpLogFile, mEditorManager->Initialize());

	CheckReturn(mpLogFile, CreateRenderer());

	return true;
}

void Engine::CleanUp() {
	if (mRenderer) {
		mRenderer.reset();
		mRenderer = nullptr;
	}
	
	if (mhRendererLibModule) {
		FreeLibrary(mhRendererLibModule);
		mhRendererLibModule = nullptr;
	}
}

bool Engine::Run() {
	MSG msg{};

	while (msg.message != WM_QUIT) {
		// If there are Window messages then process them
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff
		else {
			CheckReturn(mpLogFile, Input());
			CheckReturn(mpLogFile, Update());
			CheckReturn(mpLogFile, Draw());
		}
	}

	return true;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid
	return Engine::GetInstance()->MsgProc(hWnd, msg, wParam, lParam);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Engine::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.
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
	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE: {
		// Save the new client area dimensions.
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
			// Restoring from minimized state?
			if (mbMinimized) {
				//if (sleepable) mPowerManager->SetMode(FALSE);

				mbAppPaused = FALSE;
				mbMinimized = FALSE;
				OnResize(width, height);
			}

			// Restoring from maximized state?
			else if (mbMaximized) {
				//if (sleepable) mPowerManager->SetMode(FALSE);

				mbAppPaused = FALSE;
				mbMaximized = FALSE;
				OnResize(width, height);
			}
			// If user is dragging the resize bars, we do not resize 
			// the buffers here because as the user continuously 
			// drags the resize bars, a stream of WM_SIZE messages are
			// sent to the window, and it would be pointless (and slow)
			// to resize for each WM_SIZE message received from dragging
			// the resize bars.  So instead, we reset after the user is 
			// done resizing the window and releases the resize bars, which 
			// sends a WM_EXITSIZEMOVE message.
			else if (mbResizing) {

			}
			// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
			else {
				OnResize(width, height);
			}
		}
		return 0;
	}
	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE: {
		mbAppPaused = TRUE;
		mbResizing = TRUE;
		return 0;
	}
	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
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
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY: {
		mbDestroyed = TRUE;
		PostQuitMessage(0);
		return 0;
	}
	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR: {
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);
	}
	// Catch this message so to prevent the window from becoming too small.
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
		ReturnFalse(mpLogFile, "Failed to register the window class");

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, static_cast<LONG>(mResolution.x), static_cast<LONG>(mResolution.y) };
	if (!AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE)) 
		ReturnFalse(mpLogFile, "Failed to get window rect");

	const INT width = R.right - R.left;
	const INT height = R.bottom - R.top;

	const INT outputWidth = GetSystemMetrics(SM_CXSCREEN);
	const INT outputHeight = GetSystemMetrics(SM_CYSCREEN);

	const INT clientPosX = static_cast<INT>((outputWidth - mResolution.x) * 0.5f);
	const INT clientPosY = static_cast<INT>((outputHeight - mResolution.y) * 0.5f);

	mhMainWnd = CreateWindowW(
		L"MyModernGameEngine", L"MyModernGameEngine",
		WS_OVERLAPPEDWINDOW,
		clientPosX, clientPosY,
		width, height,
		0, 0, mhInst, 0);
	if (!mhMainWnd) ReturnFalse(mpLogFile, "Failed to create the window");

	ShowWindow(mhMainWnd, SW_SHOW);
	if (!UpdateWindow(mhMainWnd)) ReturnFalse(mpLogFile, "Failed to update window");

	return true;
}

bool Engine::CreateRenderer() {
	mhRendererLibModule = LoadLibraryW(L"Renderer.dll");
	if (!mhRendererLibModule)
		ReturnFalse(mpLogFile, "Failed to load Renderer.dll");

	CreateRendererFunc createFunc = (CreateRendererFunc)GetProcAddress(
		mhRendererLibModule, "CreateRenderer");
	DestroyRendererFunc destroyFunc = (DestroyRendererFunc)GetProcAddress(
		mhRendererLibModule, "DestroyRenderer");
	if (!createFunc || !destroyFunc)
		ReturnFalse(mpLogFile, "Failed to find Renderer.dll functions");

	mRenderer = std::unique_ptr<Renderer, RendererDeleter>(
		createFunc(), destroyFunc);
	
	CheckReturn(mpLogFile, mRenderer->Initialize(
		mpLogFile, mhMainWnd, mResolution.x, mResolution.y, [this]() { 
			this->mEditorManager->Render();
		}));

	return true;
}

bool Engine::OnResize(unsigned width, unsigned height) {
	if (mResolution.x == width && mResolution.y == height) return true;
	mResolution = Uint2(width, height);

	CheckReturn(mpLogFile, mRenderer->OnResize(width, height));

	return true;
}

bool Engine::Input() {
	CheckReturn(mpLogFile, mInputManager->Update());

	return true;
}

bool Engine::Update() {
	CheckReturn(mpLogFile, mRenderer->Update(0.f));

	return true;
}

bool Engine::Draw() {
	CheckReturn(mpLogFile, mRenderer->Draw());

	return true;
}
