#pragma once

#include "ScriptManager.hpp"

class CMoveFreeCameraScript : public CScript {
	DECLARE_SCRIPT(CMoveFreeCameraScript);

public:
	CMoveFreeCameraScript();
	virtual ~CMoveFreeCameraScript();

public:
	virtual bool Update(float dt) override;

public:
	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	CLONE(CMoveFreeCameraScript);

private:
	const float WalkSpeed = 0.5f;
	const float RunSpeed = 5.f;
	const float SprintSpeed = 10.f;
};

REGISTER_SCRIPT(CMoveFreeCameraScript);