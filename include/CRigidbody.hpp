#pragma once

#include "Component.hpp"

class CRigidbody : public Component {
public:
	CRigidbody();
	virtual ~CRigidbody();

public:
    virtual bool Initialize() override;
    virtual bool Begin() override;

    virtual bool Update(float dt) override;
    virtual bool FixedUpdate(float dt) override;
    virtual bool LateUpdate(float dt) override;

    virtual bool Final() override;

	virtual bool OnLoaded() override;
	virtual bool OnUnloaded() override;

public:

    __forceinline constexpr ERigidbody::Type GetRigidbodyType() const noexcept;

    __forceinline bool IsDynamic() const noexcept;

    __forceinline float GetMass() const noexcept;
    __forceinline float GetInvMass() const noexcept;

    __forceinline const Vec3& GetLinearVelocity() const noexcept;
    __forceinline const Vec3& GetAngularVelocity() const noexcept;
    __forceinline const Vec3& GetLocalInertia() const noexcept;
    __forceinline const Vec3& GetLocalInvInertia() const noexcept;

    __forceinline float GetLinearDamping() const noexcept;
    __forceinline float GetAngularDamping() const noexcept;

    __forceinline float GetRestitution() const noexcept;
    __forceinline float GetFriction() const noexcept;

    __forceinline bool GetUseGravity() const noexcept;

    __forceinline bool IsTrigger() const noexcept;

    __forceinline ERigidbodyConstraint::Type GetConstraints() const noexcept;

    __forceinline bool IsInertiaDirty() const noexcept;

    __forceinline Vec3 ConsumeForceAccum() noexcept;

    __forceinline void SetMass(float mass);

    __forceinline void SetLinearVelocity(const Vec3& v) noexcept;
    __forceinline void SetAngularVelocity(const Vec3& v) noexcept;

    __forceinline void SetLocalInertia(const Vec3& v) noexcept;
    __forceinline void SetLocalInvInertia(const Vec3& v) noexcept;

    __forceinline void SetLinearDamping(float v) noexcept;
    __forceinline void SetAngularDamping(float v) noexcept;

    __forceinline void SetRestitution(float v) noexcept;
    __forceinline void SetFriction(float v) noexcept;

    __forceinline void SetUseGravity(bool v) noexcept;

    __forceinline void SetTrigger(bool v) noexcept;

    __forceinline void SetConstraints(ERigidbodyConstraint::Type v) noexcept;

    __forceinline void MarkInertiaDirty() noexcept;
    __forceinline void ClearInertiaDirty() noexcept;

    __forceinline void SetType(ERigidbody::Type type);

	__forceinline void SetSleeping(bool sleeping) noexcept;
	__forceinline bool IsSleeping() const noexcept;

	__forceinline void SetSleepTimer(float timer) noexcept;
	__forceinline float GetSleepTimer() const noexcept;

public:
	CLONE(CRigidbody);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
    void AddForce(const Vec3& force);
    void AddImpulse(const Vec3& impulse);

    Vec3 ConsumeTorqueAccum() noexcept;
    void AddTorque(const Vec3& t) noexcept;

    void SetBoxInertia(float mass, const Vec3& fullExtents);
    void SetSphereInertia(float mass, float radius);
    void SetCapsuleInertiaApprox(float mass, float radius, float halfSegment);

    void SetRigidbodyType(ERigidbody::Type type) noexcept;

private:
    ERigidbody::Type mType;

    bool mInertiaDirty = true;

    float mMass;
    float mInvMass;

    Vec3 mLinearVelocity;
    Vec3 mAngularVelocity;

    Vec3 mForceAccum;
    Vec3 mTorqueAccum;

    Vec3 mLocalInertia;
    Vec3 mLocalInvInertia;

    float mLinearDamping;
    float mAngularDamping;

    float mRestitution; // 반발력
    float mFriction;

    bool mUseGravity;
    bool mIsTrigger;
    ERigidbodyConstraint::Type mConstraints;

    bool mSleeping;
    float mSleepTimer;
};

#include "CRigidbody.inl"