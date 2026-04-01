#pragma once

#include "CRenderComponent.hpp"

class CMeshRender : public CRenderComponent {
public:
	CMeshRender();
	virtual ~CMeshRender();

public:
	virtual bool Initialize() override;
	virtual bool Final() override;

	virtual bool CreateMaterial() override;

public:
	CLONE(CMeshRender);

private:

};