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
	__forceinline void SetEnvironmentCubeMap(Ptr<ATexture> environmentCubeMap);
	__forceinline Ptr<ATexture> GetEnvironmentCubeMap() const;

public:
	CLONE(CSkySphereRender);

private:
	Ptr<ATexture> mEnvironmentCubeMap;
};

#include "CSkySphereRender.inl"