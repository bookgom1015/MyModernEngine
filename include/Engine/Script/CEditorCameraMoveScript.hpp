#pragma once

#include "Engine/ScriptManager.hpp"

class CEditorCameraMoveScript : public CScript {
	DECLARE_SCRIPT(CEditorCameraMoveScript);

public:
	CEditorCameraMoveScript();
	virtual ~CEditorCameraMoveScript();

public:
	virtual bool Update(float dt) override;

public:
	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	CLONE(CEditorCameraMoveScript);

private:
	const float WalkSpeed = 0.5f;
	const float RunSpeed = 5.f;
	const float SprintSpeed = 10.f;
};

REGISTER_SCRIPT(CEditorCameraMoveScript);