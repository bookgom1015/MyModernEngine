#include "pch.h"
#include "Script/CMoveFreeCameraScript.hpp"

#include "InputManager.hpp"
#include "TimeManager.hpp"

#include "CTransform.hpp"

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
		Vec2 mouseDir = INPUT_MANAGER->GetMouseDir();
		rot.y += mouseDir.x * DegToRad * 0.25f;
		rot.x += mouseDir.y * DegToRad * 0.25f;
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