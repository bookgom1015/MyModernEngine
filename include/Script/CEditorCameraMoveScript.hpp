#pragma once

#include "ScriptManager.hpp"

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
	const float WalkSpeed = 25.f;
	const float RunSpeed = 100.f;
	const float SprintSpeed = 400.f;
};

REGISTER_SCRIPT(CEditorCameraMoveScript);