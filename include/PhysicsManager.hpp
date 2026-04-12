#pragma once

class CRigidbody;
class CCollider;
class CSphereCollider;
class CBoxCollider;
class CCapsuleCollider;

#include "PhysicsUtil.hpp"

class PhysicsManager : public Singleton<PhysicsManager> {
	SINGLETON(PhysicsManager);

public:
	bool Initialize();

	bool FixedUpdate(float dt);

	bool Final();

public:
	void RegisterRigidbody(CRigidbody* rigidbody);
	void UnregisterRigidbody(CRigidbody* rigidbody);
	bool IsRigidbodyRegistered(CRigidbody* rigidbody) const;

	void RegisterCollider(CCollider* collider);
	void UnregisterCollider(CCollider* collider);
	bool IsColliderRegistered(CCollider* collider) const;

	void Clear();

private:
	void StepSimulation(float dt);

	void BuildBodyEntries();
	void IntegrateBodies(float dt);
	void SolveCollisions(float dt);
	void SubmitDebugDraw();

	void GenerateContacts(std::vector<Contact>& outContacts);
	void ResolvePenetrations(std::vector<Contact>& contacts);
	void ResolveImpulses(std::vector<Contact>& contacts, float dt);

	void SyncPhysicsTransforms();

private:
	std::vector<CRigidbody*> mRigidbodies;
	std::vector<CCollider*> mColliders;
	std::vector<PhysicsBodyEntry> mBodies;

	std::unordered_set<CCollider*> mCollidedColliders;

	float mAccumulatedTime;

	Vec3 mGravity;
};

#ifndef PHYSICS_MANAGER
#define PHYSICS_MANAGER PhysicsManager::GetInstance()
#endif // PHYSICS_MANAGER