#pragma once

#include "Component.hpp"

class CTransform : public Component {
public:
	CTransform();
	virtual ~CTransform();

public:
	virtual bool Final() override;

public:
	CLONE(CTransform);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	const Vec3& GetWorldScale() const;

public:
	__forceinline const Vec3& GetRelativePosition() const noexcept;
	__forceinline const Vec3& GetRelativeRotation() const noexcept;
	__forceinline const Vec3& GetRelativeScale() const noexcept;

	__forceinline void SetRelativePosition(const Vec3& position);
	__forceinline void SetRelativeRotation(const Vec3& rotation);
	__forceinline void SetRelativeScale(const Vec3& scale);	

	__forceinline const Mat4& GetWorldMatrix() const noexcept;
					
	__forceinline ETrasnformDependency::Type GetDependency() const noexcept;
	__forceinline void SetDependency(ETrasnformDependency::Type dependency);

	__forceinline const Vec3& GetDirection(ETransformDirection::Type dir) const;

	__forceinline bool IsChanged() const noexcept;
	__forceinline void ReflectedChanges() noexcept;

private:
	Vec3 mPosition;
	Vec3 mRotation;
	Vec3 mScale;

	Vec3 mDirections[ETransformDirection::Count];

	Mat4 mWoldMatrix;

	ETrasnformDependency::Type mDependency;

	bool mbChanged;
};

#include "CTransform.inl"