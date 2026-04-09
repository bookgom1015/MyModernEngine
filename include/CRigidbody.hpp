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

public:
    __forceinline void SetLinearVelocity(const Vec3& v);
    __forceinline void SetAngularVelocity(const Vec3& v);

    __forceinline const Vec3& GetLinearVelocity() const noexcept;
    __forceinline const Vec3& GetAngularVelocity() const noexcept;

    __forceinline float GetMass() const noexcept;
    __forceinline float GetInvMass() const noexcept;

    __forceinline void SetMass(float mass);
    __forceinline void SetUseGravity(bool enable);
    __forceinline void SetIsTrigger(bool trigger);
    __forceinline void SetType(ERigidbody::Type type);

    __forceinline bool IsDynamic() const noexcept;
    __forceinline bool IsStatic() const noexcept;
    __forceinline bool IsKinematic() const noexcept;

    __forceinline bool GetUseGravity() const noexcept;
    __forceinline Vec3 ConsumeForceAccum() noexcept;

public:
	CLONE(CRigidbody);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
    void AddForce(const Vec3& force);
    void AddImpulse(const Vec3& impulse);

private:
    ERigidbody::Type mType;

    float mMass;
    float mInvMass;

    Vec3 mLinearVelocity;
    Vec3 mAngularVelocity;

    Vec3 mForceAccum;
    Vec3 mTorqueAccum;

    float mLinearDamping;
    float mAngularDamping;

    float mRestitution; // 반발력
    float mFriction;

    bool mUseGravity;
    bool mIsTrigger;
    ERigidbodyConstraint::Type mConstraints;
};

#include "CRigidbody.inl"