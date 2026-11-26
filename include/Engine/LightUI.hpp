#pragma once

#include "Engine/ComponentUI.hpp"

class LightUI : public ComponentUI {
public:
	LightUI();
	virtual ~LightUI();

public:
	virtual void DrawUI() override;
};