#pragma once

#include "ComponentUI.hpp"

class LightUI : public ComponentUI {
public:
	LightUI();
	virtual ~LightUI();

public:
	virtual void DrawUI() override;
};