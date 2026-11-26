#pragma once

#include "Engine/Component.hpp"

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
	Vec3 GetWorldScale() const;

	void UpdateWorldMatrixImmediate();
	void IntegrateAngularVelocityWorld(const Vec3& angularVelocityWorld, float dt);

	Mat4 GetTRMatrix() const;

public:
	__forceinline const Vec3& GetRelativePosition() const noexcept;
	__forceinline const Vec3& GetRelativeRotation() const noexcept;
	__forceinline const Vec3& GetRelativeScale() const noexcept;

	__forceinline void SetRelativePosition(const Vec3& position);
	__forceinline void SetRelativeRotation(const Vec3& rotation);
	void SetRelativeScale(const Vec3& scale);

	__forceinline void AddRelativePosition(const Vec3& delta);
	__forceinline void AddRelativeRotation(const Vec3& delta);

	__forceinline const Mat4& GetWorldMatrix() const noexcept;
	__forceinline const Mat4& GetPrevWorldMatrix() const noexcept;

	__forceinline ETrasnformDependency::Type GetDependency() const noexcept;
	__forceinline void SetDependency(ETrasnformDependency::Type dependency);

	__forceinline const Vec3& GetDirection(ETransformDirection::Type dir) const;

	__forceinline TransformTRS GetTRS() const;

private:
	void SyncQuaternionFromEuler();

private:
	Vec3 mPosition;
	Vec3 mRotation; // 에디터에 표시하는 오일러 각입니다. 단위는 도입니다.
	Vec3 mScale;

	Vec3 mDirections[ETransformDirection::Count];

	Mat4 mWorldMatrix;
	Mat4 mPrevWorldMatrix;

	Quat mRotationQ; // 런타임 내부에서 사용하는 회전입니다.

	ETrasnformDependency::Type mDependency;
};

#include "Engine/CTransform.inl"
