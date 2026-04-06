#pragma once

#include "EditorUI.hpp"

class Preference : public EditorUI {
public:
	Preference();
	virtual ~Preference();

public:
	virtual void DrawUI() override;	

private:
	void EnvironmentPanel();
	void DiffuseIrradianceInput();
	void SpecularIrradianceInput();

	void SelectDiffuseIrradianceTexture(DWORD_PTR ptr);
	void SelectSpecularIrradianceTexture(DWORD_PTR ptr);
};