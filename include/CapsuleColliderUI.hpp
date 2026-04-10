#pragma once

#include "ComponentUI.hpp"

class CapsuleColliderUI : public ComponentUI {
public:
	CapsuleColliderUI();
	virtual ~CapsuleColliderUI();

public:
	virtual void DrawUI() override;

public:
	void OffsetPanel();
	void RadiusPanel();
	void HalfSegmentPanel();
	void TriggerPanel();
};