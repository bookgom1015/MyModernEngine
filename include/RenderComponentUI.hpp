#pragma once

#include "ComponentUI.hpp"

class RenderComponentUI : public ComponentUI {
public:
	RenderComponentUI(EComponent::Type type, const std::string& name);
	virtual ~RenderComponentUI();

protected:
	virtual void TargetChanged() override;

protected:
	void MeshPanel();
	void MaterialSlotPanel();
	void MaterialPanel();
	void TexturePanel();

private:
	void SelectMesh(DWORD_PTR ptr);
	void SelectMaterial(DWORD_PTR ptr);
	void SelectTexture(DWORD_PTR ptr);

private:
	int mSelectedMaterialIdx;
};