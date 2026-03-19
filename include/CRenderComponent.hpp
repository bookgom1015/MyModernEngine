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
	virtual bool Render() = 0;

	virtual bool CreateMaterial() = 0;

public:
	Ptr<AMaterial> CreateDynamicMaterial();

public:
	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

	__forceinline Ptr<AMesh> GetMesh() const noexcept;
	__forceinline void SetMesh(Ptr<AMesh> mesh) noexcept;

	__forceinline Ptr<AMaterial> GetMaterial() const noexcept;
	__forceinline void SetMaterial(Ptr<AMaterial> material) noexcept;

	__forceinline Ptr<AMaterial> GetSharedMaterial() const noexcept;

private:
	Ptr<AMesh> mMesh;

	Ptr<AMaterial> mMaterial;
	Ptr<AMaterial> mSharedMaterial;
	Ptr<AMaterial> mDynamicMaterial;
};

#include "CRenderComponent.inl"