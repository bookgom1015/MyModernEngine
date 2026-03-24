#include "pch.h"
#include "Script/CEditorCameraMoveScript.hpp"

CEditorCameraMoveScript::CEditorCameraMoveScript() : CScript() {}

CEditorCameraMoveScript::~CEditorCameraMoveScript() {}

bool CEditorCameraMoveScript::Update(float dt) {
	return true;
}

bool CEditorCameraMoveScript::SaveToLevelFile(FILE* const pFile) {
	return true;
}

bool CEditorCameraMoveScript::LoadFromLevelFile(FILE* const pFile) {
	return true;
}