#pragma once

class CRigidbody;
class CCollider;
class CSphereCollider;
class CBoxCollider;
class CCapsuleCollider;

struct PhysicsBodyEntry {
	CRigidbody* Rigidbody = nullptr;
	CCollider* Collider = nullptr;
};

class PhysicsManager : public Singleton<PhysicsManager> {
	SINGLETON(PhysicsManager);

public:
	bool Initialize();

	bool FixedUpdate(float dt);

	bool Final();

public:
	void RegisterRigidbody(CRigidbody* rigidbody);
	void UnregisterRigidbody(CRigidbody* rigidbody);

	void RegisterCollider(CCollider* collider);
	void UnregisterCollider(CCollider* collider);

	void Clear();

private:
	void StepSimulation(float dt);

	void BuildBodyEntries();
	void IntegrateBodies(float dt);
	void SolveCollisions(float dt);
	void SubmitDebugDraw();

private:
	std::vector<CRigidbody*> mRigidbodies;
	std::vector<CCollider*> mColliders;
	std::vector<PhysicsBodyEntry> mBodies;

	float mAccumulatedTime;

	Vec3 mGravity;
};

#ifndef PHYSICS_MANAGER
#define PHYSICS_MANAGER PhysicsManager::GetInstance()
#endif // PHYSICS_MANAGER