#pragma once

#include "CCollider.hpp"

class CCapsuleCollider : public CCollider {
public:
	CCapsuleCollider();
	virtual ~CCapsuleCollider();

public:
	virtual bool Initialize() override;
	virtual bool Final() override;

	virtual bool OnLoaded() override;
	virtual bool OnUnloaded() override;

public:
	CLONE(CCapsuleCollider);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	__forceinline float GetRadius() const noexcept;
	__forceinline float GetHalfSegment() const noexcept;
	__forceinline ECapsuleAxis::Type GetAxis() const noexcept;

	__forceinline void SetRadius(float radius) noexcept;
	__forceinline void SetHalfSegment(float halfSegment) noexcept;
	__forceinline void SetAxis(ECapsuleAxis::Type axis) noexcept;

private:
	float mRadius;
	float mHalfSegment;
	ECapsuleAxis::Type mAxis;
};

#include "CCapsuleCollider.inl"