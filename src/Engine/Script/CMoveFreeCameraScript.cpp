#include "Engine/pch.h"
#include "Engine/Script/CMoveFreeCameraScript.hpp"

#include "Engine/InputManager.hpp"
#include "Engine/TimeManager.hpp"

#include "Engine/CTransform.hpp"

CMoveFreeCameraScript::CMoveFreeCameraScript()
	: CScript() {}

CMoveFreeCameraScript::~CMoveFreeCameraScript() {}

bool CMoveFreeCameraScript::Update(float dt) {
	auto pos = Transform()->GetRelativePosition();
	auto rot = Transform()->GetRelativeRotation();

	auto forward = Transform()->GetDirection(ETransformDirection::E_Forward);
	auto right = Transform()->GetDirection(ETransformDirection::E_Right);

	float speed = RunSpeed;
	if (KEY_PRESSED(EKey::E_LSHIFT))
		speed = SprintSpeed;
	else if (KEY_PRESSED(EKey::E_CTRL))
		speed = WalkSpeed;

	if (KEY_PRESSED(EKey::E_W))
		pos += forward * speed * dt;
	if (KEY_PRESSED(EKey::E_S))
		pos -= forward * speed * dt;
	if (KEY_PRESSED(EKey::E_A))
		pos -= right * speed * dt;
	if (KEY_PRESSED(EKey::E_D))
		pos += right * speed * dt;

	if (KEY_PRESSED(EKey::E_RBTN)) {
		const Vec2 mouseDir = INPUT_MANAGER->GetMouseDir();
		// 두 버튼을 함께 누른 경우 좌클릭 위젯이 커서 워프를 처리합니다.
		if (!(::GetAsyncKeyState(VK_LBUTTON) & 0x8000))
			INPUT_MANAGER->WrapMouseCursor(true);
		rot.y += mouseDir.x * 0.25f;
		rot.x += mouseDir.y * 0.25f;
	}

	Transform()->SetRelativePosition(pos);
	Transform()->SetRelativeRotation(rot);

	return true;
}

bool CMoveFreeCameraScript::SaveToLevelFile(FILE* const pFile) {
	return true;
}

bool CMoveFreeCameraScript::LoadFromLevelFile(FILE* const pFile) {
	return true;
}