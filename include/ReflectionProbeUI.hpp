#pragma once

#include "ComponentUI.hpp"

class ReflectionProbeUI : public ComponentUI {
public:
	ReflectionProbeUI();
	virtual ~ReflectionProbeUI();

public:
	virtual void DrawUI() override;
};