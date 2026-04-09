#pragma once

#include "Component.hpp"

class CCollider : public Component {
public:
	CCollider(EComponent::Type type, ECollider::Type colliderType);
	virtual ~CCollider();

public:
	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	__forceinline ECollider::Type GetColliderType() const noexcept;

	__forceinline bool IsTrigger() const noexcept;
	__forceinline void SetTrigger(bool enable);

	__forceinline Vec3 GetOffset() const noexcept;
	__forceinline void SetOffset(const Vec3& offset);

protected:
	ECollider::Type mColliderType;

	Vec3 mOffset;

	bool mbIsTrigger;
};

#include "CCollider.inl"