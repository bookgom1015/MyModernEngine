#pragma once

#include "Asset.hpp"													
#include "ATexture.hpp"

class AMaterial : public Asset {
public:
	AMaterial();
	AMaterial(const AMaterial& other);
	virtual ~AMaterial();

public:
	CLONE(AMaterial);

	virtual bool Load(const std::wstring& filePath) override;
	virtual bool Save(const std::wstring& filePath) override;

public:
	__forceinline const Ptr<ATexture>& GetAlbedoMap() const noexcept;
	__forceinline void SetAlbedoMap(const Ptr<ATexture>& albedoMap) noexcept;

	__forceinline const Vec3& GetAlbedo() const noexcept;
	__forceinline void SetAlbedo(const Vec3& albedo) noexcept;

	__forceinline const Vec3& GetSpecular() const noexcept;
	__forceinline void SetSpecular(const Vec3& specular) noexcept;

	__forceinline float GetRoughness() const noexcept;
	__forceinline void SetRoughness(float roughness) noexcept;

	__forceinline float GetMetalic() const noexcept;
	__forceinline void SetMetalic(float metalic) noexcept;

	__forceinline ERenderDomain::Type GetRenderDomain() const noexcept;
	__forceinline void SetRenderDomain(ERenderDomain::Type domain) noexcept;
																									
private:
	Ptr<ATexture> mAlbedoMap;

	Vec3 mAlbedo;
	Vec3 mSpecular;
	float mRoughness;
	float mMetalic;

	bool mpNeedToUpdate;

	ERenderDomain::Type mDomain;
};

#include "AMaterial.inl"