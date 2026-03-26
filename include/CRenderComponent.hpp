#pragma once

#include "Component.hpp"

#include "AMesh.hpp"
#include "AMaterial.hpp"

class CRenderComponent : public Component {
public:
	CRenderComponent(EComponent::Type type);
	CRenderComponent(const CRenderComponent& other);
	virtual ~CRenderComponent();

public:
	virtual bool Initialize() override;	

	virtual bool Final() override;

	virtual bool CreateMaterial() = 0;

	virtual bool OnMeshChanged();
	virtual bool OnMaterialChanged();

public:
	bool CreateDynamicMaterial();

	bool SetMesh(Ptr<AMesh> mesh) noexcept;
	bool SetMaterial(Ptr<AMaterial> material) noexcept;

	Vec3 GetAlbedo() const;
	bool SetAlbedo(Vec3 albedo);

	float GetRoughness() const;
	bool SetRoughness(float roughness);

	float GetMetalic() const;
	bool SetMetalic(float metalic);

public:
	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

	__forceinline Ptr<AMesh> GetMesh() const noexcept;
	__forceinline Ptr<AMaterial> GetMaterial() const noexcept;

private:
	Ptr<AMesh> mMesh;

	Ptr<AMaterial> mMaterial;
	Ptr<AMaterial> mSharedMaterial;
	Ptr<AMaterial> mDynamicMaterial;

};

#include "CRenderComponent.inl"