#pragma once

#include "ComponentUI.hpp"

class SphereColliderUI : public ComponentUI {
public:
	SphereColliderUI();
	virtual ~SphereColliderUI();

public:
	virtual void DrawUI() override;

public:
	void OffsetPanel();
	void TriggerPanel();
};