#pragma once

#include "CCollider.hpp"

class CBoxCollider : public CCollider {
public:
	CBoxCollider();
	virtual ~CBoxCollider();

public:
	virtual bool Initialize() override;
	virtual bool Final() override;

public:
	CLONE(CBoxCollider);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	__forceinline const Vec3& GetHalfExtents() const noexcept;
	__forceinline void SetHalfExtents(const Vec3& halfExtents) noexcept;

private:
	Vec3 mHalfExtents;
};

#include "CBoxCollider.inl"