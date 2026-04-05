#pragma once

#include "CRenderComponent.hpp"

class CSkySphereRender : public CRenderComponent {
public:
	CSkySphereRender();
	virtual ~CSkySphereRender();

public:
	virtual bool Initialize() override;
	virtual bool Final() override;

	virtual bool CreateMaterial() override;

public:
	CLONE(CSkySphereRender);

private:

};