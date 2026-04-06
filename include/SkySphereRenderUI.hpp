#pragma once

#include "RenderComponentUI.hpp"

class SkySphereRenderUI : public RenderComponentUI {
public:
	SkySphereRenderUI();
	virtual ~SkySphereRenderUI();

public:
	virtual void DrawUI() override;

private:
	void TexturePanel();
	void SelectEnvironmentCubeMap(DWORD_PTR ptr);
};