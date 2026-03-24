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
};

REGISTER_SCRIPT(CEditorCameraMoveScript);