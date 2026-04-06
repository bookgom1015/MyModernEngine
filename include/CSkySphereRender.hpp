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

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

private:
	Ptr<ATexture> mEnvironmentCubeMap;
};

#include "CSkySphereRender.inl"