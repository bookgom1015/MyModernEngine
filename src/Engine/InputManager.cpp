#include "Engine/pch.h"
#include "Engine/InputManager.hpp"

#include "Engine/Engine.hpp"

namespace {
	int gKeyIndices[EKey::Count] = {
		'Q',
		'W',
		'E',
		'R',
		'A',
		'S',
		'D',
		'F',
		'Z',
		'X',
		'C',
		'V',

		VK_LBUTTON,
		VK_RBUTTON,
		VK_MBUTTON,

		VK_LEFT,
		VK_RIGHT,
		VK_UP,
		VK_DOWN,
		VK_RETURN,
		VK_MENU,
		VK_CONTROL,
		VK_LSHIFT,
		VK_RSHIFT,
		VK_SPACE,
		VK_ESCAPE,
		VK_HOME,
		VK_END,

		VK_F1,
		VK_F2,
		VK_F3,
		VK_F4,
		VK_F5,
		VK_F6,
		VK_F7,
		VK_F8,
		VK_F9,
	};
}

InputManager::InputManager() 
	: mMousePos{}
	, mMousePrevPos{}
	, mMouseDir{}
	, mbWheelChanged{}
	, mWheel{}
	, mbActivated{ true } {}

InputManager::~InputManager() {}

bool InputManager::Initialize() {
	mRegisteredKeys.resize(EKey::Count);

	return true;
}

bool InputManager::Update() {
	UpdateKeysStates();
	UpdateMouseState();

	return true;
}

void InputManager::UpdateKeysStates() {
	if (GetFocus() == Engine::GetInstance()->GetMainWndHandle() && mbActivated) {
		for (int i = 0; i < EKey::Count; ++i) {
			auto& key = mRegisteredKeys[i];
			if (GetAsyncKeyState(gKeyIndices[i])) {
				if (key.Pressed) key.State = EKeyState::E_Pressed;
				else key.State = EKeyState::E_Tapped;

				key.Pressed = true;
			}
			else {
				if (key.Pressed) key.State = EKeyState::E_Released;
				else EKeyState::E_None;

				key.Pressed = false;
			}
		}
	}
	else {
		for (int i = 0; i < EKey::Count; ++i) {
			GetAsyncKeyState(gKeyIndices[i]);

			auto& key = mRegisteredKeys[i];
			if (key.State == EKeyState::E_Tapped
				|| key.State == EKeyState::E_Pressed)
				key.State = EKeyState::E_Released;
			else
				key.State = EKeyState::E_None;

			key.Pressed = false;
		}
	}
}

void InputManager::WrapMouseCursor(bool wrapVertically) {
	if (mPendingMouseWarp.x != 0.f || mPendingMouseWarp.y != 0.f)
		return;

	POINT cursor{};
	if (!::GetCursorPos(&cursor))
		return;

	const HMONITOR handle = ::MonitorFromPoint(cursor, MONITOR_DEFAULTTONULL);
	MONITORINFO monitor{};
	monitor.cbSize = sizeof(monitor);
	if (!handle || !::GetMonitorInfo(handle, &monitor))
		return;

	constexpr long EdgeMargin = 2;
	constexpr long WarpInset = 8;
	const auto wrapAxis = [](long value, long low, long high) {
		if (high - low <= 2 * WarpInset)
			return value;
		if (value < low + EdgeMargin)
			return high - 1 - WarpInset;
		if (value >= high - EdgeMargin)
			return low + WarpInset;
		return value;
	};

	const RECT& bounds = monitor.rcMonitor;
	POINT destination{ wrapAxis(cursor.x, bounds.left, bounds.right), cursor.y };
	if (wrapVertically)
		destination.y = wrapAxis(cursor.y, bounds.top, bounds.bottom);
	if (destination.x == cursor.x && destination.y == cursor.y)
		return;

	RECT clip{};
	if (::GetClipCursor(&clip) && ::PtInRect(&clip, destination)
		&& ::SetCursorPos(destination.x, destination.y)) {
		mPendingMouseWarp = Vec2(
			static_cast<float>(destination.x - cursor.x),
			static_cast<float>(destination.y - cursor.y));
	}
}

void InputManager::UpdateMouseState() {
	POINT point{};
	if (::GetCursorPos(&point) && ::ScreenToClient(Engine::GetInstance()->GetMainWndHandle(), &point)) {
		mMousePrevPos = mMousePos;
		mMousePos = Vec2(static_cast<float>(point.x), static_cast<float>(point.y));
		// 다음 유효한 입력 샘플에서 커서 워프에 의한 변위만 한 번 제외합니다.
		mMouseDir = mMousePos - mMousePrevPos - mPendingMouseWarp;
		mPendingMouseWarp = {};
	}
	else {
		mMouseDir = {};
	}
	
	if (mbWheelChanged) mbWheelChanged = false;
	else mWheel = 0;
}