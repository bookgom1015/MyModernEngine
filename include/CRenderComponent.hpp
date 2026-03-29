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

public:
	__forceinline Ptr<AMesh> GetMesh() const noexcept;
	bool SetMesh(Ptr<AMesh> mesh) noexcept;

	__forceinline Ptr<AMaterial> GetMaterial() const noexcept;
	bool SetMaterial(Ptr<AMaterial> material) noexcept;

	Vec3 GetAlbedo() const;
	bool SetAlbedo(Vec3 albedo);

	float GetRoughness() const;
	bool SetRoughness(float roughness);

	float GetMetalic() const;
	bool SetMetalic(float metalic);

	float GetSpecular() const;
	bool SetSpecular(float specular);

	Ptr<ATexture> GetAlbedoMap() const;
	bool SetAlbedoMap(Ptr<ATexture> albedoMap);

	Ptr<ATexture> GetNormalMap() const;
	bool SetNormalMap(Ptr<ATexture> normalMap);

public:
	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;


private:
	Ptr<AMesh> mMesh;

	Ptr<AMaterial> mMaterial;
	Ptr<AMaterial> mSharedMaterial;
	Ptr<AMaterial> mDynamicMaterial;

};

#include "CRenderComponent.inl"