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

using namespace DirectX;

namespace {
	constexpr float kMaxStepDt = 1.f / 30.f;
	constexpr int   kVelocityIterations = 10;
	constexpr int   kPositionIterations = 4;
}

PhysicsManager::PhysicsManager()
	: mRigidbodies{}
	, mColliders{}
	, mBodies{}
	, mAccumulatedTime{ 0.f }
	, mGravity{ 0.f, -9.8f, 0.f } {}

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
	if (rigidbody == nullptr) return;
	if (std::find(mRigidbodies.begin(), mRigidbodies.end(), rigidbody) != mRigidbodies.end()) return;
	mRigidbodies.push_back(rigidbody);
}

void PhysicsManager::UnregisterRigidbody(CRigidbody* rigidbody) {
	if (rigidbody == nullptr) return;
	std::erase(mRigidbodies, rigidbody);
}

bool PhysicsManager::IsRigidbodyRegistered(CRigidbody* rigidbody) const {
	if (rigidbody == nullptr) return false;
	return std::find(mRigidbodies.begin(), mRigidbodies.end(), rigidbody) != mRigidbodies.end();
}

void PhysicsManager::RegisterCollider(CCollider* collider) {
	if (collider == nullptr) return;
	if (std::find(mColliders.begin(), mColliders.end(), collider) != mColliders.end()) return;
	mColliders.push_back(collider);
}

void PhysicsManager::UnregisterCollider(CCollider* collider) {
	if (collider == nullptr) return;
	std::erase(mColliders, collider);
}

bool PhysicsManager::IsColliderRegistered(CCollider* collider) const {
	if (collider == nullptr) return false;
	return std::find(mColliders.begin(), mColliders.end(), collider) != mColliders.end();
}

void PhysicsManager::Clear() {
	mRigidbodies.clear();
	mColliders.clear();
	mBodies.clear();
	mCollidedColliders.clear();
	mAccumulatedTime = 0.f;
}

void PhysicsManager::StepSimulation(float dt) {
	if (dt <= 0.f) {
		return;
	}

	dt = std::min(dt, kMaxStepDt);
	mCollidedColliders.clear();

	BuildBodyEntries();
	SyncPhysicsTransforms();
	IntegrateBodies(dt);
	SyncPhysicsTransforms();

	std::vector<Contact> contacts;
	contacts.reserve(mBodies.size() * 2);
	GenerateContacts(contacts);

	if (contacts.empty()) {
		SyncPhysicsTransforms();
		return;
	}

	for (int i = 0; i < kPositionIterations; ++i) {
		ResolvePenetrations(contacts);
		SyncPhysicsTransforms();
		contacts.clear();
		GenerateContacts(contacts);
		if (contacts.empty()) {
			break;
		}
	}

	if (!contacts.empty()) {
		ResolveImpulses(contacts, dt);
	}

	SyncPhysicsTransforms();
}

void PhysicsManager::BuildBodyEntries() {
	mBodies.clear();
	mBodies.reserve(mColliders.size());

	for (CCollider* collider : mColliders) {
		if (collider == nullptr) continue;

		GameObject* owner = collider->GetOwner();
		if (owner == nullptr) continue;

		PhysicsBodyEntry body{};
		body.Collider = collider;
		body.Rigidbody = owner->Rigidbody().Get();
		mBodies.push_back(body);
	}

	for (PhysicsBodyEntry& body : mBodies) {
		if (body.Rigidbody != nullptr && body.Rigidbody->IsInertiaDirty()) {
			PhysicsUtil::UpdateBodyInertiaFromCollider(body);
		}
	}
}

void PhysicsManager::IntegrateBodies(float dt) {
	for (CRigidbody* rb : mRigidbodies) {
		if (rb == nullptr || !rb->IsDynamic()) continue;
		if (rb->GetInvMass() <= 0.f) continue;

		CTransform* transform = rb->Transform();
		if (transform == nullptr) continue;

		CCollider* collider = rb->GetOwner() ? rb->GetOwner()->GetColliderComponent().Get() : nullptr;
		const bool isBox = (collider != nullptr && collider->GetColliderType() == ECollider::E_Box);

		Vec3 linearVelocity = rb->GetLinearVelocity();
		Vec3 angularVelocity = rb->GetAngularVelocity();

		if (rb->GetUseGravity()) {
			linearVelocity += mGravity * dt;
		}

		linearVelocity += rb->ConsumeForceAccum() * rb->GetInvMass() * dt;
		angularVelocity += PhysicsUtil::MulInvInertiaWorld(rb, rb->ConsumeTorqueAccum()) * dt;

		linearVelocity *= std::max(0.f, 1.f - rb->GetLinearDamping() * dt);
		angularVelocity *= std::max(0.f, 1.f - rb->GetAngularDamping() * dt);

		float maxAngularSpeed = TwoPI * 4.f;
		if (isBox) {
			maxAngularSpeed = TwoPI * 2.f;
		}

		const float angularLen = angularVelocity.Length();
		if (angularLen > maxAngularSpeed && angularLen > PhysicsUtil::kEpsilon) {
			angularVelocity *= (maxAngularSpeed / angularLen);
		}

		rb->SetLinearVelocity(linearVelocity);
		rb->SetAngularVelocity(angularVelocity);

		transform->AddRelativePosition(linearVelocity * dt);

		transform->IntegrateAngularVelocityWorld(angularVelocity, dt);
	}
}

void PhysicsManager::SolveCollisions(float dt) {
	UNREFERENCED_PARAMETER(dt);
}

void PhysicsManager::SubmitDebugDraw() {
	for (CCollider* collider : mColliders) {
		if (collider == nullptr) continue;

		DebugColliderShape desc{};
		desc.Type = collider->GetColliderType();

		const Vec3 localScale = collider->GetScale();
		const Mat4 trans = XMMatrixTranslation(collider->GetOffset().x, collider->GetOffset().y, collider->GetOffset().z);
		const Mat4 scale = XMMatrixScaling(localScale.x, localScale.y, localScale.z);
		desc.World = scale * trans * collider->Transform()->GetWorldMatrix();

		switch (collider->GetColliderType()) {
		case ECollider::E_Sphere:
			desc.Radius = static_cast<CSphereCollider*>(collider)->GetRadius();
			break;
		case ECollider::E_Box:
			desc.HalfExtents = static_cast<CBoxCollider*>(collider)->GetHalfExtents();
			break;
		case ECollider::E_Capsule:
			desc.Radius = static_cast<CCapsuleCollider*>(collider)->GetRadius();
			desc.HalfSegment = static_cast<CCapsuleCollider*>(collider)->GetHalfSegment();
			break;
		default:
			break;
		}

		RENDERER->AddDebugColliderShape(desc);
	}
}

void PhysicsManager::GenerateContacts(std::vector<Contact>& outContacts) {
	const size_t bodyCount = mBodies.size();

	for (size_t i = 0; i < bodyCount; ++i) {
		for (size_t j = i + 1; j < bodyCount; ++j) {
			PhysicsBodyEntry& a = mBodies[i];
			PhysicsBodyEntry& b = mBodies[j];

			if (a.Collider == nullptr || b.Collider == nullptr) continue;
			if (a.Collider->IsTrigger() || b.Collider->IsTrigger()) continue;

			const bool aDynamic = (a.Rigidbody != nullptr && a.Rigidbody->IsDynamic() && a.Rigidbody->GetInvMass() > 0.f);
			const bool bDynamic = (b.Rigidbody != nullptr && b.Rigidbody->IsDynamic() && b.Rigidbody->GetInvMass() > 0.f);
			if (!aDynamic && !bDynamic) continue;

			Vec3 normal(0.f, 1.f, 0.f);
			float penetration = 0.f;
			bool hit = false;

			const ECollider::Type typeA = a.Collider->GetColliderType();
			const ECollider::Type typeB = b.Collider->GetColliderType();

			if (typeA == ECollider::E_Sphere && typeB == ECollider::E_Sphere) {
				hit = PhysicsUtil::CheckSphereSphere(static_cast<CSphereCollider*>(a.Collider), static_cast<CSphereCollider*>(b.Collider), normal, penetration);
			}
			else if (typeA == ECollider::E_Sphere && typeB == ECollider::E_Box) {
				hit = PhysicsUtil::CheckSphereBox(static_cast<CSphereCollider*>(a.Collider), static_cast<CBoxCollider*>(b.Collider), normal, penetration);
				if (hit) normal = -normal;
			}
			else if (typeA == ECollider::E_Box && typeB == ECollider::E_Sphere) {
				hit = PhysicsUtil::CheckSphereBox(static_cast<CSphereCollider*>(b.Collider), static_cast<CBoxCollider*>(a.Collider), normal, penetration);
			}
			else if (typeA == ECollider::E_Box && typeB == ECollider::E_Box) {
				hit = PhysicsUtil::CheckBoxBox(static_cast<CBoxCollider*>(a.Collider), static_cast<CBoxCollider*>(b.Collider), normal, penetration);
				if (hit) {
					PhysicsUtil::GenerateBoxBoxManifold(a, b, normal, penetration, outContacts);
					mCollidedColliders.insert(a.Collider);
					mCollidedColliders.insert(b.Collider);
					continue;
				}
			}
			else if (typeA == ECollider::E_Capsule && typeB == ECollider::E_Sphere) {
				hit = PhysicsUtil::CheckCapsuleSphere(static_cast<CCapsuleCollider*>(a.Collider), static_cast<CSphereCollider*>(b.Collider), normal, penetration);
			}
			else if (typeA == ECollider::E_Sphere && typeB == ECollider::E_Capsule) {
				hit = PhysicsUtil::CheckCapsuleSphere(static_cast<CCapsuleCollider*>(b.Collider), static_cast<CSphereCollider*>(a.Collider), normal, penetration);
				if (hit) normal = -normal;
			}
			else if (typeA == ECollider::E_Capsule && typeB == ECollider::E_Capsule) {
				hit = PhysicsUtil::CheckCapsuleCapsule(static_cast<CCapsuleCollider*>(a.Collider), static_cast<CCapsuleCollider*>(b.Collider), normal, penetration);
			}
			else if (typeA == ECollider::E_Capsule && typeB == ECollider::E_Box) {
				hit = PhysicsUtil::CheckCapsuleBox(static_cast<CCapsuleCollider*>(a.Collider), static_cast<CBoxCollider*>(b.Collider), normal, penetration);
				if (hit) normal = -normal;
			}
			else if (typeA == ECollider::E_Box && typeB == ECollider::E_Capsule) {
				hit = PhysicsUtil::CheckCapsuleBox(static_cast<CCapsuleCollider*>(b.Collider), static_cast<CBoxCollider*>(a.Collider), normal, penetration);
			}

			if (!hit) continue;

			Contact c{};
			c.A = &a;
			c.B = &b;
			c.Normal = PhysicsUtil::NormalizeSafe(normal);
			c.Penetration = penetration;
			c.Point = PhysicsUtil::ComputeContactPoint(a, b, c.Normal);
			outContacts.push_back(c);

			mCollidedColliders.insert(a.Collider);
			mCollidedColliders.insert(b.Collider);
		}
	}
}

void PhysicsManager::ResolvePenetrations(std::vector<Contact>& contacts) {
	for (Contact& c : contacts) {
		if (c.A == nullptr || c.B == nullptr) continue;
		PhysicsUtil::PositionalCorrection(*c.A, *c.B, c.Normal, c.Penetration);
	}
}

void PhysicsManager::ResolveImpulses(std::vector<Contact>& contacts, float dt) {
	UNREFERENCED_PARAMETER(dt);

	std::unordered_map<PairKey, std::vector<Contact*>, PairKeyHash> manifolds;
	manifolds.reserve(contacts.size());

	for (Contact& c : contacts) {
		manifolds[PhysicsUtil::MakePairKey(c)].push_back(&c);
	}

	for (int iteration = 0; iteration < kVelocityIterations; ++iteration) {
		for (auto& [key, manifold] : manifolds) {
			UNREFERENCED_PARAMETER(key);
			if (manifold.empty())
				continue;

			PhysicsBodyEntry& a = *manifold[0]->A;
			PhysicsBodyEntry& b = *manifold[0]->B;

			float normalImpulseSum = 0.f;
			Vec3 avgPoint(0.f);
			Vec3 avgNormal(0.f);

			for (Contact* cp : manifold) {
				if (cp == nullptr)
					continue;

				// restitution은 첫 iteration에서만 허용
				const float restitutionScale = (iteration == 0) ? 1.f : 0.f;

				normalImpulseSum += PhysicsUtil::ApplyNormalImpulseAtContact(
					a, b, *cp, restitutionScale);

				avgPoint += cp->Point;
				avgNormal += cp->Normal;
			}

			const float count = static_cast<float>(manifold.size());
			if (count <= 0.f)
				continue;

			avgPoint /= count;
			avgNormal = PhysicsUtil::NormalizeSafe(avgNormal / count);

			const bool isBoxBox = PhysicsUtil::IsBoxBoxPair(a, b);
			const bool useSinglePointFriction = !isBoxBox && manifold.size() > 0;

			if (useSinglePointFriction) {
				PhysicsUtil::ApplyPairFrictionImpulse(
					a, b, avgPoint, avgNormal, normalImpulseSum);
			}
			else {
				float frictionScale = 1.f;
				if (isBoxBox && manifold.size() <= 2) {
					frictionScale = 0.35f;
				}

				const float perContactImpulse = (count > 0.f)
					? (normalImpulseSum * frictionScale / count)
					: 0.f;

				for (Contact* cp : manifold) {
					if (cp == nullptr)
						continue;
					PhysicsUtil::ApplyPairFrictionImpulse(
						a, b, cp->Point, cp->Normal, perContactImpulse);
				}

				if (isBoxBox && manifold.size() <= 2) {
					if (a.Rigidbody && a.Rigidbody->IsDynamic()) {
						Vec3 w = a.Rigidbody->GetAngularVelocity();
						a.Rigidbody->SetAngularVelocity(w * 0.92f);
					}
					if (b.Rigidbody && b.Rigidbody->IsDynamic()) {
						Vec3 w = b.Rigidbody->GetAngularVelocity();
						b.Rigidbody->SetAngularVelocity(w * 0.92f);
					}
				}
			}
		}
	}
}

void PhysicsManager::SyncPhysicsTransforms() {
	for (CRigidbody* rb : mRigidbodies) {
		if (rb == nullptr) continue;
		if (CTransform* t = rb->Transform()) {
			t->UpdateWorldMatrixImmediate();
		}
	}

	for (CCollider* collider : mColliders) {
		if (collider == nullptr) continue;
		if (CTransform* t = collider->Transform()) {
			t->UpdateWorldMatrixImmediate();
		}
	}
}
