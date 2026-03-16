#include "pch.h"
#include "InputManager.hpp"

#include "Engine.hpp"

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

void InputManager::UpdateMouseState() {
	POINT point{};
	GetCursorPos(&point);
	ScreenToClient(Engine::GetInstance()->GetMainWndHandle(), &point);

	mMousePrevPos = mMousePos;
	mMousePos = Vec2(static_cast<float>(point.x), static_cast<float>(point.y));

	mMouseDir = mMousePos - mMousePrevPos;
	
	if (mbWheelChanged) mbWheelChanged = false;
	else mWheel = 0;
}