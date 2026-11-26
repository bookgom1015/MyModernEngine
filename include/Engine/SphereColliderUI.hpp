#pragma once

#include "Engine/ComponentUI.hpp"

class SphereColliderUI : public ComponentUI {
public:
	SphereColliderUI();
	virtual ~SphereColliderUI();

public:
	virtual void DrawUI() override;

public:
	void OffsetPanel();
	void RadiusPanel();
	void TriggerPanel();
};