#pragma once

#include "ComponentUI.hpp"

class AddComponentButton : public ComponentUI {
public:
	AddComponentButton();
	virtual ~AddComponentButton();

public:
	virtual void DrawUI() override;

public:
	void SelectComponent(DWORD_PTR listUI);
	void SelectScript(DWORD_PTR listUI);

private:
	void AddComponent();
	void AddScript();

private:
	float mButtonsWidthAccum;
};