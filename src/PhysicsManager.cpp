#include "pch.h"
#include "PhysicsManager.hpp"

#include RENDERER_HEADER

#include "GameObject.hpp"

#include "CTransform.hpp"
#include "CRigidbody.hpp"
#include "CCollider.hpp"
#include "CBoxCollider.hpp"
#include "CSphereCollider.hpp"
#include "CCapsuleCollider.hpp"

namespace {
	bool CheckSphereSphere(
		const CSphereCollider* a
		, const CSphereCollider* b
		, Vec3& outNormal
		, float& outPenetration) {
		const Vec3 pa = a->GetOwner()->Transform()->GetRelativePosition() + a->GetOffset();
		const Vec3 pb = b->GetOwner()->Transform()->GetRelativePosition() + b->GetOffset();

		Vec3 ab = pb - pa;
		float distSq = ab.LengthSquared();

		const float ra = a->GetRadius();
		const float rb = b->GetRadius();
		const float r = ra + rb;

		if (distSq >= r * r)
			return false;

		float dist = sqrtf(std::max(distSq, 0.000001f));
		outNormal = (dist > 0.000001f) ? (ab / dist) : Vec3(0.f, 1.f, 0.f);
		outPenetration = r - dist;
		return true;
	}
}

PhysicsManager::PhysicsManager() 
	: mRigidbodies{}
	, mColliders{}
	, mBodies{}
	, mAccumulatedTime{}
	, mGravity{0.f, -9.8f, 0.f} {}

PhysicsManager::~PhysicsManager() {}

bool PhysicsManager::Initialize() {
	mAccumulatedTime = 0.f;

	return true;
}

bool PhysicsManager::FixedUpdate(float dt) {
	StepSimulation(dt);

	return true;
}

bool PhysicsManager::Final() {
	SubmitDebugDraw();

	return true;
}

void PhysicsManager::RegisterRigidbody(CRigidbody* rigidbody) {
	if (rigidbody == nullptr)
		return;

	auto iter = std::find(mRigidbodies.begin(), mRigidbodies.end(), rigidbody);
	if (iter != mRigidbodies.end()) return;

	mRigidbodies.push_back(rigidbody);
}

void PhysicsManager::UnregisterRigidbody(CRigidbody* rigidbody) {
	if (rigidbody == nullptr) return;

	std::erase(mRigidbodies, rigidbody);
}

void PhysicsManager::RegisterCollider(CCollider* collider) {
	if (collider == nullptr) return;

	auto iter = std::find(mColliders.begin(), mColliders.end(), collider);
	if (iter != mColliders.end()) return;

	mColliders.push_back(collider);
}

void PhysicsManager::UnregisterCollider(CCollider* collider) {
	if (collider == nullptr) return;

	std::erase(mColliders, collider);
}

void PhysicsManager::Clear() {
	mRigidbodies.clear();
	mColliders.clear();
	mBodies.clear();

	mAccumulatedTime = 0.f;
}

void PhysicsManager::StepSimulation(float dt) {
	BuildBodyEntries();
	IntegrateBodies(dt);
	SolveCollisions(dt);
}

void PhysicsManager::BuildBodyEntries() {
	mBodies.clear();
	mBodies.reserve(mColliders.size());

	for (CCollider* collider : mColliders) {
		if (collider == nullptr) continue;

		GameObject* owner = collider->GetOwner();
		if (owner == nullptr) continue;

		CRigidbody* rigidbody = owner->Rigidbody().Get();

		PhysicsBodyEntry entry{};
		entry.Rigidbody = rigidbody;
		entry.Collider = collider;

		mBodies.push_back(entry);
	}
}

void PhysicsManager::IntegrateBodies(float dt) {
	for (CRigidbody* rb : mRigidbodies) {
		if (rb == nullptr) continue;
		if (!rb->IsDynamic()) continue;

		CTransform* transform = rb->Transform();
		if (transform == nullptr) continue;

		Vec3 velocity = rb->GetLinearVelocity();

		if (rb->GetUseGravity()) {
			velocity += mGravity * dt;
		}

		velocity += rb->ConsumeForceAccum() * rb->GetInvMass() * dt;

		transform->AddRelativePosition(velocity * dt);

		rb->SetLinearVelocity(velocity);
	}
}

void PhysicsManager::SolveCollisions(float dt) {
	UNREFERENCED_PARAMETER(dt);

	const size_t count = mBodies.size();

	for (size_t i = 0; i < count; ++i) {
		for (size_t j = i + 1; j < count; ++j) {
			PhysicsBodyEntry& a = mBodies[i];
			PhysicsBodyEntry& b = mBodies[j];

			if (a.Collider == nullptr || b.Collider == nullptr)
				continue;

			if (a.Rigidbody == nullptr && b.Rigidbody == nullptr)
				continue;

			Vec3 normal{};
			float penetration = 0.f;
			bool hit = false;

			if (a.Collider->GetColliderType() == ECollider::E_Sphere &&
				b.Collider->GetColliderType() == ECollider::E_Sphere) {
				hit = CheckSphereSphere(
					static_cast<CSphereCollider*>(a.Collider),
					static_cast<CSphereCollider*>(b.Collider),
					normal, penetration);
			}

			if (!hit)
				continue;

			const bool aDynamic = (a.Rigidbody != nullptr && a.Rigidbody->IsDynamic());
			const bool bDynamic = (b.Rigidbody != nullptr && b.Rigidbody->IsDynamic());

			if (!aDynamic && !bDynamic)
				continue;

			CTransform* ta = a.Collider->Transform();
			CTransform* tb = b.Collider->Transform();

			if (ta == nullptr || tb == nullptr)
				continue;

			if (a.Collider->IsTrigger() || b.Collider->IsTrigger())
				continue;

			if (aDynamic && bDynamic) {
				ta->AddRelativePosition(-normal * (penetration * 0.5f));
				tb->AddRelativePosition(normal * (penetration * 0.5f));

				Vec3 va = a.Rigidbody->GetLinearVelocity();
				Vec3 vb = b.Rigidbody->GetLinearVelocity();

				float vaN = va.Dot(normal);
				float vbN = vb.Dot(normal);

				va += normal * (vbN - vaN);
				vb += normal * (vaN - vbN);

				a.Rigidbody->SetLinearVelocity(va);
				b.Rigidbody->SetLinearVelocity(vb);
			}
			else if (aDynamic) {
				ta->AddRelativePosition(-normal * penetration);

				Vec3 va = a.Rigidbody->GetLinearVelocity();
				float vaN = va.Dot(normal);
				if (vaN > 0.f)
					va -= normal * vaN;

				a.Rigidbody->SetLinearVelocity(va);
			}
			else if (bDynamic) {
				tb->AddRelativePosition(normal * penetration);

				Vec3 vb = b.Rigidbody->GetLinearVelocity();
				float vbN = vb.Dot(normal);
				if (vbN < 0.f)
					vb -= normal * vbN;

				b.Rigidbody->SetLinearVelocity(vb);
			}
		}
	}
}

void PhysicsManager::SubmitDebugDraw() {
	for (CCollider* collider : mColliders) {
		if (collider == nullptr)
			continue;

		DebugColliderShape desc{};
		desc.Type = collider->GetColliderType();
		desc.World = collider->Transform()->GetWorldMatrix();

		switch (collider->GetColliderType()) {
		case ECollider::E_Sphere: {
			auto sphere = static_cast<CSphereCollider*>(collider);
			desc.Radius = sphere->GetRadius();
			break;
		}
		case ECollider::E_Box: {
			auto box = static_cast<CBoxCollider*>(collider);
			desc.HalfExtents = box->GetHalfExtents();
			break;
		}
		case ECollider::E_Capsule: {
			auto capsule = static_cast<CCapsuleCollider*>(collider);
			desc.Radius = capsule->GetRadius();
			desc.HalfSegment = capsule->GetHalfSegment();
			break;
		}
		default:
			break;
		}

		RENDERER->AddDebugColliderShape(desc);
	}
}
