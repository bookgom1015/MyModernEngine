#pragma once

#include "Engine/ComponentUI.hpp"

class BoxColliderUI : public ComponentUI {
public:
	BoxColliderUI();
	virtual ~BoxColliderUI();

public:
	virtual void DrawUI() override;

public:
	void OffsetPanel();
	void ScalePanel();
	void TriggerPanel();
};