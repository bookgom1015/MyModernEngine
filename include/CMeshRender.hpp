#pragma once

#include "CRenderComponent.hpp"

#include "AMesh.hpp"
#include "AMaterial.hpp"

class CMeshRender : public CRenderComponent {
public:
	CMeshRender();
	virtual ~CMeshRender();

public:
	virtual bool Initialize() override;
	virtual bool Update(float dt) override;
	virtual bool Final() override;

	virtual bool CreateMaterial() override;
	virtual bool OnMeshChanged() override;
	virtual bool OnMaterialChanged() override;

public:
	CLONE(CMeshRender);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

private:

};