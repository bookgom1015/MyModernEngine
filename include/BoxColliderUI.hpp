#pragma once

#include "ComponentUI.hpp"

class BoxColliderUI : public ComponentUI {
public:
	BoxColliderUI();
	virtual ~BoxColliderUI();

public:
	virtual void DrawUI() override;

public:
	void OffsetPanel();
	void TriggerPanel();
};