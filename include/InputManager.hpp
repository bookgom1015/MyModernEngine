#pragma once

namespace EKey {
	enum Type {
		E_Q,
		E_W,
		E_E,
		E_R,
		E_A,
		E_S,
		E_D,
		E_F,
		E_Z,
		E_X,
		E_C,
		E_V,

		E_LBTN,
		E_RBTN,
		E_MBTN,

		E_LEFT,
		E_RIGHT,
		E_UP,
		E_DOWN,

		E_ENTER,
		E_ALT,
		E_CTRL,
		E_LSHIFT,
		E_RSHIFT,
		E_SPACE,
		E_ESC,

		E_HOME,
		E_END,

		E_F1, 
		E_F2, 
		E_F3,
		E_F4, 
		E_F5,
		E_F6, 
		E_F7,
		E_F8, 
		E_F9,

		Count
	};
}

namespace EKeyState {
	enum Type {
		E_None,
		E_Pressed,
		E_Released,
		E_Tapped,
	};
}

struct KeyInfo {
	EKeyState::Type State;
	bool Pressed;
};

class InputManager : public Singleton<InputManager> {
	SINGLETON(InputManager);

public:
	bool Initialize();
	bool Update();

public:
	__forceinline EKeyState::Type GetKeyState(EKey::Type key) const;
	__forceinline const Vec2& GetMousePos() const;
	__forceinline const Vec2& GetMousePrevPos() const;
	__forceinline const Vec2& GetMouseDir() const;

	__forceinline bool IsActivated() const;
	__forceinline void Activate(bool state);

	__forceinline int GetMouseWheel();
	__forceinline void SetMouseWheel(int state);

private:
	void UpdateKeysStates();
	void UpdateMouseState();

private:
	std::vector<KeyInfo> mRegisteredKeys;

	Vec2 mMousePos;
	Vec2 mMousePrevPos;
	Vec2 mMouseDir;

	bool mbWheelChanged;
	int mWheel;

	bool mbActivated;
};

#include "InputManager.inl"

#ifndef INPUT_MANAGER
#define INPUT_MANAGER InputManager::GetInstance()
#endif // INPUT_MANAGER

#define KEY_CHECK(__Key, __State) INPUT_MANAGER->GetKeyState(__Key) == __State

#define KEY_TAP(__Key)		KEY_CHECK(__Key, EKeyState::E_Tapped)
#define KEY_PRESSED(__Key)	KEY_CHECK(__Key, EKeyState::E_Pressed)
#define KEY_RELEASED(__Key)	KEY_CHECK(__Key, EKeyState::E_Released)
#define KEY_NONE(__Key)		KEY_CHECK(__Key, EKeyState::E_None)
