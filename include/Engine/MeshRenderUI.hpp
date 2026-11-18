#pragma once

#include "Engine/RenderComponentUI.hpp"

class MeshRenderUI : public RenderComponentUI {
public:
	MeshRenderUI();
	virtual ~MeshRenderUI();

public:
	virtual void DrawUI() override;
};