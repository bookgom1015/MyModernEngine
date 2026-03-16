#ifndef __INPUTMANAGER_INL__
#define __INPUTMANAGER_INL__

EKeyState::Type InputManager::GetKeyState(EKey::Type key) const {
	return mRegisteredKeys[key].State;
}

const Vec2& InputManager::GetMousePos() const { return mMousePos; }

const Vec2& InputManager::GetMousePrevPos() const { return mMousePrevPos; }

const Vec2& InputManager::GetMouseDir() const { return mMouseDir; }

bool InputManager::IsActivated() const { return mbActivated; }

void InputManager::Activate(bool state) { mbActivated = state; }

int InputManager::GetMouseWheel() { return mWheel; }

void InputManager::SetMouseWheel(int state) {
	mWheel = state;
	mbWheelChanged = true;
}

#endif // __INPUTMANAGER_INL__