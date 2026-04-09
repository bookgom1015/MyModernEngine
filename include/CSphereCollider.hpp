#pragma once

#include "CCollider.hpp"

class CSphereCollider : public CCollider {
public:
	CSphereCollider();
	virtual ~CSphereCollider();

public:
	virtual bool Initialize() override;
	virtual bool Final() override;

public:
	CLONE(CSphereCollider);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	__forceinline float GetRadius() const noexcept;
	__forceinline void SetRadius(float radius) noexcept;

private:
	float mRadius;
};

#include "CSphereCollider.inl"