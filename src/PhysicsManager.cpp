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
	static constexpr float kEpsilon = 0.000001f;

	struct Segment {
		Vec3 A;
		Vec3 B;
	};

	struct OBB {
		Vec3 Center;
		Vec3 Axis[3];      // 정규화된 월드 축
		Vec3 HalfExtents;  // 각 축 방향 half extent
	};

	static Vec3 GetColliderWorldPosition(const CCollider* collider) {
		auto transform = collider ? collider->GetOwner()->Transform() : nullptr;
		if (transform == nullptr)
			return collider ? collider->GetOffset() : Vec3(0.f);

		// 기존 RelativePosition 대신 월드 기준으로 맞추는 게 안전
		const Mat4 world = transform->GetWorldMatrix();
		return Vec3::Transform(collider->GetOffset(), world);
	}

	static Vec3 GetFinalColliderScale(const CCollider* collider) {
		const Vec3 local = collider->GetScale();

		auto t = collider->GetOwner()->Transform();
		if (t == nullptr)
			return local;

		// 가능하면 GetWorldScale() 쓰는 편이 더 맞음
		const Vec3 world = t->GetRelativeScale();

		return Vec3(
			local.x * world.x,
			local.y * world.y,
			local.z * world.z
		);
	}

	static Vec3 GetBoxScaledHalfExtents(const CBoxCollider* box) {
		const Vec3 s = GetFinalColliderScale(box);
		const Vec3 he = box->GetHalfExtents();
		return Vec3(he.x * s.x, he.y * s.y, he.z * s.z);
	}

	static float GetSphereScaledRadius(const CSphereCollider* sphere) {
		const Vec3 s = GetFinalColliderScale(sphere);
		const float maxScale = std::max({ s.x, s.y, s.z });
		return sphere->GetRadius() * maxScale;
	}

	static float GetCapsuleScaledRadius(const CCapsuleCollider* capsule) {
		const Vec3 s = GetFinalColliderScale(capsule);

		float axisScale = 1.f;

		switch (capsule->GetAxis()) {
		case ECapsuleAxis::E_XAxis: axisScale = std::max(s.y, s.z); break;
		case ECapsuleAxis::E_YAxis: axisScale = std::max(s.x, s.z); break;
		case ECapsuleAxis::E_ZAxis: axisScale = std::max(s.x, s.y); break;
		}

		return capsule->GetRadius() * axisScale;
	}

	static Vec3 NormalizeSafe(const Vec3& v, const Vec3& fallback = Vec3(0.f, 1.f, 0.f)) {
		const float lenSq = v.LengthSquared();
		if (lenSq <= kEpsilon)
			return fallback;

		Vec3 out = v;
		out /= sqrtf(lenSq);
		return out;
	}

	static Vec3 AbsVec3(const Vec3& v) {
		return Vec3(fabsf(v.x), fabsf(v.y), fabsf(v.z));
	}

	static float GetComponent(const Vec3& v, int idx) {
		switch (idx) {
		case 0: return v.x;
		case 1: return v.y;
		default: return v.z;
		}
	}

	static Vec3 GetAxisByIndex(const OBB& box, int idx) {
		return box.Axis[idx];
	}

	static OBB GetOBB(const CBoxCollider* box) {
		OBB out{};

		out.Center = GetColliderWorldPosition(box);
		out.HalfExtents = GetBoxScaledHalfExtents(box);

		const Mat4 world = box->GetOwner()->Transform()->GetWorldMatrix();

		out.Axis[0] = NormalizeSafe(Vec3::TransformNormal(Vec3(1.f, 0.f, 0.f), world), Vec3(1.f, 0.f, 0.f));
		out.Axis[1] = NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 1.f, 0.f), world), Vec3(0.f, 1.f, 0.f));
		out.Axis[2] = NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), world), Vec3(0.f, 0.f, 1.f));

		return out;
	}

	static Vec3 WorldToOBBLocalPoint(const OBB& box, const Vec3& worldPoint) {
		const Vec3 d = worldPoint - box.Center;
		return Vec3(
			d.Dot(box.Axis[0]),
			d.Dot(box.Axis[1]),
			d.Dot(box.Axis[2])
		);
	}

	static Vec3 OBBLocalToWorldPoint(const OBB& box, const Vec3& localPoint) {
		return box.Center
			+ box.Axis[0] * localPoint.x
			+ box.Axis[1] * localPoint.y
			+ box.Axis[2] * localPoint.z;
	}

	static Vec3 ClosestPointOnOBB(const OBB& box, const Vec3& worldPoint) {
		Vec3 local = WorldToOBBLocalPoint(box, worldPoint);

		local.x = std::clamp(local.x, -box.HalfExtents.x, box.HalfExtents.x);
		local.y = std::clamp(local.y, -box.HalfExtents.y, box.HalfExtents.y);
		local.z = std::clamp(local.z, -box.HalfExtents.z, box.HalfExtents.z);

		return OBBLocalToWorldPoint(box, local);
	}

	static bool CheckSphereSphere(
		const CSphereCollider* a
		, const CSphereCollider* b
		, Vec3& outNormal
		, float& outPenetration) {
		const Vec3 pa = GetColliderWorldPosition(a);
		const Vec3 pb = GetColliderWorldPosition(b);

		Vec3 ab = pb - pa;
		float distSq = ab.LengthSquared();

		const float ra = GetSphereScaledRadius(a);
		const float rb = GetSphereScaledRadius(b);
		const float r = ra + rb;

		if (distSq >= r * r)
			return false;

		float dist = sqrtf(std::max(distSq, 0.000001f));
		outNormal = (dist > 0.000001f) ? (ab / dist) : Vec3(0.f, 1.f, 0.f);
		outPenetration = r - dist;
		return true;
	}

	static bool CheckSphereBox(
		const CSphereCollider* sphere
		, const CBoxCollider* box
		, Vec3& outNormal
		, float& outPenetration) {
		const Vec3 sphereCenter = GetColliderWorldPosition(sphere);
		const float radius = GetSphereScaledRadius(sphere);

		const OBB obb = GetOBB(box);

		const Vec3 sphereLocal = WorldToOBBLocalPoint(obb, sphereCenter);

		Vec3 closestLocal(
			std::clamp(sphereLocal.x, -obb.HalfExtents.x, obb.HalfExtents.x),
			std::clamp(sphereLocal.y, -obb.HalfExtents.y, obb.HalfExtents.y),
			std::clamp(sphereLocal.z, -obb.HalfExtents.z, obb.HalfExtents.z)
		);

		const Vec3 closestWorld = OBBLocalToWorldPoint(obb, closestLocal);

		Vec3 diff = sphereCenter - closestWorld;
		const float distSq = diff.LengthSquared();

		if (distSq > radius * radius)
			return false;

		// 내부 침투 처리
		if (distSq <= kEpsilon) {
			const float dx = obb.HalfExtents.x - fabsf(sphereLocal.x);
			const float dy = obb.HalfExtents.y - fabsf(sphereLocal.y);
			const float dz = obb.HalfExtents.z - fabsf(sphereLocal.z);

			if (dx <= dy && dx <= dz)
				outNormal = obb.Axis[0] * ((sphereLocal.x >= 0.f) ? 1.f : -1.f);
			else if (dy <= dx && dy <= dz)
				outNormal = obb.Axis[1] * ((sphereLocal.y >= 0.f) ? 1.f : -1.f);
			else
				outNormal = obb.Axis[2] * ((sphereLocal.z >= 0.f) ? 1.f : -1.f);

			outNormal = NormalizeSafe(outNormal);
			outPenetration = radius;
			return true;
		}

		const float dist = sqrtf(distSq);
		outNormal = diff / dist;
		outPenetration = radius - dist;
		return true;
	}

	static bool CheckBoxBox(
		const CBoxCollider* a
		, const CBoxCollider* b
		, Vec3& outNormal
		, float& outPenetration) {
		const OBB A = GetOBB(a);
		const OBB B = GetOBB(b);

		float R[3][3];
		float AbsR[3][3];

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				R[i][j] = A.Axis[i].Dot(B.Axis[j]);
				AbsR[i][j] = fabsf(R[i][j]) + 1e-6f;
			}
		}

		const Vec3 tWorld = B.Center - A.Center;
		const Vec3 t(
			tWorld.Dot(A.Axis[0]),
			tWorld.Dot(A.Axis[1]),
			tWorld.Dot(A.Axis[2])
		);

		float minOverlap = FLT_MAX;
		Vec3 bestAxis(0.f, 1.f, 0.f);

		auto TryAxis = [&](const Vec3& axisWorld, float dist, float ra, float rb) -> bool {
			const float overlap = (ra + rb) - fabsf(dist);
			if (overlap <= 0.f)
				return false;

			Vec3 axis = NormalizeSafe(axisWorld);
			if ((B.Center - A.Center).Dot(axis) < 0.f)
				axis = -axis;

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
			}
			return true;
			};

		// A의 3축
		for (int i = 0; i < 3; ++i) {
			const float ra = GetComponent(A.HalfExtents, i);
			const float rb =
				B.HalfExtents.x * AbsR[i][0] +
				B.HalfExtents.y * AbsR[i][1] +
				B.HalfExtents.z * AbsR[i][2];

			if (!TryAxis(A.Axis[i], GetComponent(t, i), ra, rb))
				return false;
		}

		// B의 3축
		for (int j = 0; j < 3; ++j) {
			const float ra =
				A.HalfExtents.x * AbsR[0][j] +
				A.HalfExtents.y * AbsR[1][j] +
				A.HalfExtents.z * AbsR[2][j];

			const float rb = GetComponent(B.HalfExtents, j);

			const float dist = fabsf(
				t.x * R[0][j] +
				t.y * R[1][j] +
				t.z * R[2][j]
			);

			if (!TryAxis(B.Axis[j], dist, ra, rb))
				return false;
		}

		// 교차축 9개
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				Vec3 axis = A.Axis[i].Cross(B.Axis[j]);
				if (axis.LengthSquared() <= kEpsilon)
					continue;

				const int i1 = (i + 1) % 3;
				const int i2 = (i + 2) % 3;
				const int j1 = (j + 1) % 3;
				const int j2 = (j + 2) % 3;

				const float ra =
					GetComponent(A.HalfExtents, i1) * AbsR[i2][j] +
					GetComponent(A.HalfExtents, i2) * AbsR[i1][j];

				const float rb =
					GetComponent(B.HalfExtents, j1) * AbsR[i][j2] +
					GetComponent(B.HalfExtents, j2) * AbsR[i][j1];

				const float dist = fabsf(
					GetComponent(t, i2) * R[i1][j] -
					GetComponent(t, i1) * R[i2][j]
				);

				if (!TryAxis(axis, dist, ra, rb))
					return false;
			}
		}

		outNormal = bestAxis;
		outPenetration = minOverlap;
		return true;
	}

	static float Clamp01(float v) {
		return std::clamp(v, 0.f, 1.f);
	}

	static void ClosestPtSegmentSegment(
		const Vec3& p1
		, const Vec3& q1
		, const Vec3& p2
		, const Vec3& q2
		, float& s
		, float& t
		, Vec3& c1
		, Vec3& c2) {
		const Vec3 d1 = q1 - p1; // S1 방향
		const Vec3 d2 = q2 - p2; // S2 방향
		const Vec3 r = p1 - p2;
		const float a = d1.Dot(d1);
		const float e = d2.Dot(d2);
		const float f = d2.Dot(r);

		if (a <= 0.000001f && e <= 0.000001f) {
			s = t = 0.f;
			c1 = p1;
			c2 = p2;
			return;
		}

		if (a <= 0.000001f) {
			s = 0.f;
			t = Clamp01(f / e);
		}
		else {
			const float c = d1.Dot(r);

			if (e <= 0.000001f) {
				t = 0.f;
				s = Clamp01(-c / a);
			}
			else {
				const float b = d1.Dot(d2);
				const float denom = a * e - b * b;

				if (denom != 0.f)
					s = Clamp01((b * f - c * e) / denom);
				else
					s = 0.f;

				t = (b * s + f) / e;

				if (t < 0.f) {
					t = 0.f;
					s = Clamp01(-c / a);
				}
				else if (t > 1.f) {
					t = 1.f;
					s = Clamp01((b - c) / a);
				}
			}
		}

		c1 = p1 + d1 * s;
		c2 = p2 + d2 * t;
	}

	static float GetCapsuleScaledHalfSegment(const CCapsuleCollider* capsule) {
		const Vec3 s = GetFinalColliderScale(capsule);

		float axisScale = 1.f;

		switch (capsule->GetAxis()) {
		case ECapsuleAxis::E_XAxis: axisScale = s.x; break;
		case ECapsuleAxis::E_YAxis: axisScale = s.y; break;
		case ECapsuleAxis::E_ZAxis: axisScale = s.z; break;
		}

		return capsule->GetHalfSegment() * axisScale;
	}

	static Segment GetCapsuleSegment(const CCapsuleCollider* capsule) {
		const Vec3 center = GetColliderWorldPosition(capsule);
		const float halfSeg = GetCapsuleScaledHalfSegment(capsule);

		Vec3 axis(0.f, 1.f, 0.f);
		switch (capsule->GetAxis()) {
		case ECapsuleAxis::E_XAxis: axis = Vec3(1.f, 0.f, 0.f); break;
		case ECapsuleAxis::E_YAxis: axis = Vec3(0.f, 1.f, 0.f); break;
		case ECapsuleAxis::E_ZAxis: axis = Vec3(0.f, 0.f, 1.f); break;
		}

		axis = Vec3::TransformNormal(axis, capsule->GetOwner()->Transform()->GetWorldMatrix());
		axis.Normalize();

		Segment seg{};
		seg.A = center - axis * halfSeg;
		seg.B = center + axis * halfSeg;
		return seg;
	}

	static Vec3 ClosestPointOnSegment(const Vec3& p, const Vec3& a, const Vec3& b) {
		Vec3 ab = b - a;
		float abLenSq = ab.LengthSquared();
		if (abLenSq <= 0.000001f)
			return a;

		float t = (p - a).Dot(ab) / abLenSq;
		t = std::clamp(t, 0.f, 1.f);
		return a + ab * t;
	}


	static Vec3 GetBodyWorldCenter(const PhysicsBodyEntry& body) {
		if (body.Collider != nullptr)
			return GetColliderWorldPosition(body.Collider);

		if (body.Rigidbody != nullptr && body.Rigidbody->Transform() != nullptr)
			return body.Rigidbody->Transform()->GetRelativePosition();

		return Vec3(0.f);
	}

	static Vec3 MulInvInertiaWorld(const CRigidbody* rb, const Vec3& v) {
		if (rb == nullptr || !rb->IsDynamic())
			return Vec3(0.f);

		auto transform = rb->GetOwner()->Transform();
		if (transform == nullptr)
			return Vec3(0.f);

		const Mat4 world = transform->GetWorldMatrix();
		const Vec3 ax = NormalizeSafe(Vec3::TransformNormal(Vec3(1.f, 0.f, 0.f), world), Vec3(1.f, 0.f, 0.f));
		const Vec3 ay = NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 1.f, 0.f), world), Vec3(0.f, 1.f, 0.f));
		const Vec3 az = NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), world), Vec3(0.f, 0.f, 1.f));

		const Vec3 invI = rb->GetLocalInvInertia();
		const float lx = v.Dot(ax);
		const float ly = v.Dot(ay);
		const float lz = v.Dot(az);

		return ax * (lx * invI.x)
			+ ay * (ly * invI.y)
			+ az * (lz * invI.z);
	}

	static float ComputeImpulseDenominator(
		CRigidbody* rbA, CRigidbody* rbB,
		float invMassA, float invMassB,
		const Vec3& ra, const Vec3& rb,
		const Vec3& dir) {
		float denom = invMassA + invMassB;

		if (invMassA > 0.f && rbA != nullptr) {
			const Vec3 raXn = ra.Cross(dir);
			denom += raXn.Dot(MulInvInertiaWorld(rbA, raXn));
		}

		if (invMassB > 0.f && rbB != nullptr) {
			const Vec3 rbXn = rb.Cross(dir);
			denom += rbXn.Dot(MulInvInertiaWorld(rbB, rbXn));
		}

		return denom;
	}

	static float GetBodyInvMass(const PhysicsBodyEntry& body) {
		if (body.Rigidbody == nullptr) return 0.f;
		if (!body.Rigidbody->IsDynamic()) return 0.f;
		return body.Rigidbody->GetInvMass();
	}

	static void ApplyImpulseAtPoint(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Vec3& point,
		const Vec3& impulse) {
		CRigidbody* rbA = a.Rigidbody;
		CRigidbody* rbB = b.Rigidbody;

		const float invMassA = GetBodyInvMass(a);
		const float invMassB = GetBodyInvMass(b);

		const Vec3 centerA = GetBodyWorldCenter(a);
		const Vec3 centerB = GetBodyWorldCenter(b);
		const Vec3 ra = point - centerA;
		const Vec3 rb = point - centerB;

		if (invMassA > 0.f && rbA != nullptr) {
			rbA->SetLinearVelocity(rbA->GetLinearVelocity() - impulse * invMassA);

			Vec3 ang = rbA->GetAngularVelocity();
			ang += MulInvInertiaWorld(rbA, ra.Cross(-impulse));
			rbA->SetAngularVelocity(ang);
		}

		if (invMassB > 0.f && rbB != nullptr) {
			rbB->SetLinearVelocity(rbB->GetLinearVelocity() + impulse * invMassB);

			Vec3 ang = rbB->GetAngularVelocity();
			ang += MulInvInertiaWorld(rbB, rb.Cross(impulse));
			rbB->SetAngularVelocity(ang);
		}
	}

	static bool CheckCapsuleSphere(
		const CCapsuleCollider* capsule
		, const CSphereCollider* sphere
		, Vec3& outNormal
		, float& outPenetration) {
		const Segment seg = GetCapsuleSegment(capsule);
		const Vec3 sphereCenter = GetColliderWorldPosition(sphere);

		const Vec3 closest = ClosestPointOnSegment(sphereCenter, seg.A, seg.B);
		Vec3 diff = sphereCenter - closest;
		const float distSq = diff.LengthSquared();

		const float rCapsule = GetCapsuleScaledRadius(capsule);
		const float rSphere = GetSphereScaledRadius(sphere);
		const float r = rCapsule + rSphere;

		if (distSq >= r * r)
			return false;

		const float dist = sqrtf(std::max(distSq, 0.000001f));
		outNormal = (dist > 0.000001f) ? (diff / dist) : Vec3(0.f, 1.f, 0.f);
		outPenetration = r - dist;
		return true;
	}

	static bool CheckCapsuleCapsule(
		const CCapsuleCollider* a
		, const CCapsuleCollider* b
		, Vec3& outNormal
		, float& outPenetration) {
		const Segment sa = GetCapsuleSegment(a);
		const Segment sb = GetCapsuleSegment(b);

		float s = 0.f, t = 0.f;
		Vec3 c1{}, c2{};
		ClosestPtSegmentSegment(sa.A, sa.B, sb.A, sb.B, s, t, c1, c2);

		Vec3 diff = c2 - c1;
		const float distSq = diff.LengthSquared();

		const float ra = GetCapsuleScaledRadius(a);
		const float rb = GetCapsuleScaledRadius(b);
		const float r = ra + rb;

		if (distSq >= r * r)
			return false;

		const float dist = sqrtf(std::max(distSq, 0.000001f));
		outNormal = (dist > 0.000001f) ? (diff / dist) : Vec3(0.f, 1.f, 0.f);
		outPenetration = r - dist;
		return true;
	}

	static bool CheckCapsuleBox(
		const CCapsuleCollider* capsule
		, const CBoxCollider* box
		, Vec3& outNormal
		, float& outPenetration) {
		const Segment seg = GetCapsuleSegment(capsule);
		const float radius = GetCapsuleScaledRadius(capsule);
		const OBB obb = GetOBB(box);

		auto TestPointAgainstOBBExpanded = [&](const Vec3& p, Vec3& n, float& pen) -> bool {
			const Vec3 pLocal = WorldToOBBLocalPoint(obb, p);

			Vec3 closestLocal(
				std::clamp(pLocal.x, -obb.HalfExtents.x, obb.HalfExtents.x),
				std::clamp(pLocal.y, -obb.HalfExtents.y, obb.HalfExtents.y),
				std::clamp(pLocal.z, -obb.HalfExtents.z, obb.HalfExtents.z)
			);

			const Vec3 closestWorld = OBBLocalToWorldPoint(obb, closestLocal);
			Vec3 diff = p - closestWorld;
			const float distSq = diff.LengthSquared();

			if (distSq > radius * radius)
				return false;

			if (distSq <= kEpsilon) {
				const float dx = obb.HalfExtents.x - fabsf(pLocal.x);
				const float dy = obb.HalfExtents.y - fabsf(pLocal.y);
				const float dz = obb.HalfExtents.z - fabsf(pLocal.z);

				if (dx <= dy && dx <= dz)
					n = obb.Axis[0] * ((pLocal.x >= 0.f) ? 1.f : -1.f);
				else if (dy <= dx && dy <= dz)
					n = obb.Axis[1] * ((pLocal.y >= 0.f) ? 1.f : -1.f);
				else
					n = obb.Axis[2] * ((pLocal.z >= 0.f) ? 1.f : -1.f);

				n = NormalizeSafe(n);
				pen = radius;
				return true;
			}

			const float dist = sqrtf(distSq);
			n = diff / dist;
			pen = radius - dist;
			return true;
			};

		Vec3 bestNormal{};
		float bestPenetration = -1.f;

		Vec3 n{};
		float pen = 0.f;

		if (TestPointAgainstOBBExpanded(seg.A, n, pen)) {
			bestNormal = n;
			bestPenetration = pen;
		}

		if (TestPointAgainstOBBExpanded(seg.B, n, pen) && pen > bestPenetration) {
			bestNormal = n;
			bestPenetration = pen;
		}

		const Vec3 mid = (seg.A + seg.B) * 0.5f;
		if (TestPointAgainstOBBExpanded(mid, n, pen) && pen > bestPenetration) {
			bestNormal = n;
			bestPenetration = pen;
		}

		if (bestPenetration < 0.f)
			return false;

		outNormal = bestNormal;
		outPenetration = bestPenetration;
		return true;
	}

	static void PositionalCorrection(
		PhysicsBodyEntry& a
		, PhysicsBodyEntry& b
		, const Vec3& normal
		, float penetration) {
		const float invMassA = GetBodyInvMass(a);
		const float invMassB = GetBodyInvMass(b);
		const float invMassSum = invMassA + invMassB;

		if (invMassSum <= 0.f)
			return;

		constexpr float kSlop = 0.005f;
		constexpr float kPercent = 0.2f;

		const float correctionMag =
			std::max(penetration - kSlop, 0.f) * kPercent / invMassSum;

		const Vec3 correction = normal * correctionMag;

		if (invMassA > 0.f) {
			if (CTransform* t = a.Collider->Transform())
				t->AddRelativePosition(-correction * invMassA);
		}

		if (invMassB > 0.f) {
			if (CTransform* t = b.Collider->Transform())
				t->AddRelativePosition(correction * invMassB);
		}
	}

	static float ComputeRestitution(CRigidbody* a, CRigidbody* b, float velAlongNormal) {
		float e = 0.f;

		if (a != nullptr && b != nullptr)      e = std::min(a->GetRestitution(), b->GetRestitution());
		else if (a != nullptr)                 e = a->GetRestitution();
		else if (b != nullptr)                 e = b->GetRestitution();

		// resting contact 에선 튀지 않게
		if (fabsf(velAlongNormal) < 0.2f)
			e = 0.f;

		return e;
	}

	static float ComputeFriction(CRigidbody* a, CRigidbody* b) {
		if (a != nullptr && b != nullptr)
			return sqrtf(a->GetFriction() * b->GetFriction());
		else if (a != nullptr)
			return a->GetFriction();
		else if (b != nullptr)
			return b->GetFriction();

		return 0.f;
	}

	static void ApplyCollisionImpulse(
		PhysicsBodyEntry& a
		, PhysicsBodyEntry& b
		, const Vec3& normal) {
		CRigidbody* rbA = a.Rigidbody;
		CRigidbody* rbB = b.Rigidbody;

		const float invMassA = GetBodyInvMass(a);
		const float invMassB = GetBodyInvMass(b);
		const float invMassSum = invMassA + invMassB;

		if (invMassSum <= 0.f)
			return;

		const Vec3 va = (rbA != nullptr) ? rbA->GetLinearVelocity() : Vec3(0.f);
		const Vec3 vb = (rbB != nullptr) ? rbB->GetLinearVelocity() : Vec3(0.f);

		const Vec3 rv = vb - va;
		const float velAlongNormal = rv.Dot(normal);

		if (velAlongNormal > 0.f)
			return;

		const float restitution = ComputeRestitution(rbA, rbB, velAlongNormal);

		const float j = -(1.f + restitution) * velAlongNormal / invMassSum;
		const Vec3 impulse = normal * j;

		if (invMassA > 0.f)
			rbA->SetLinearVelocity(rbA->GetLinearVelocity() - impulse * invMassA);

		if (invMassB > 0.f)
			rbB->SetLinearVelocity(rbB->GetLinearVelocity() + impulse * invMassB);
	}

	static void ApplyFrictionImpulse(
		PhysicsBodyEntry& a
		, PhysicsBodyEntry& b
		, const Vec3& normal) {
		CRigidbody* rbA = a.Rigidbody;
		CRigidbody* rbB = b.Rigidbody;

		const float invMassA = GetBodyInvMass(a);
		const float invMassB = GetBodyInvMass(b);
		const float invMassSum = invMassA + invMassB;

		if (invMassSum <= 0.f)
			return;

		const Vec3 va = (rbA != nullptr) ? rbA->GetLinearVelocity() : Vec3(0.f);
		const Vec3 vb = (rbB != nullptr) ? rbB->GetLinearVelocity() : Vec3(0.f);

		const Vec3 rv = vb - va;
		const float vn = rv.Dot(normal);

		Vec3 tangent = rv - normal * vn;
		const float tangentLenSq = tangent.LengthSquared();
		if (tangentLenSq <= 0.000001f)
			return;

		tangent /= sqrtf(tangentLenSq);

		const float jt = -rv.Dot(tangent) / invMassSum;
		const float mu = ComputeFriction(rbA, rbB);

		// 간단한 Coulomb 근사
		const float normalImpulseApprox = fabsf(vn) / invMassSum;
		const float maxFriction = normalImpulseApprox * mu;
		const float clampedJt = std::clamp(jt, -maxFriction, maxFriction);

		const Vec3 frictionImpulse = tangent * clampedJt;

		if (invMassA > 0.f)
			rbA->SetLinearVelocity(rbA->GetLinearVelocity() - frictionImpulse * invMassA);

		if (invMassB > 0.f)
			rbB->SetLinearVelocity(rbB->GetLinearVelocity() + frictionImpulse * invMassB);
	}

	static bool GenerateContact(
		PhysicsBodyEntry& a
		, PhysicsBodyEntry& b
		, Contact& outContact) {
		CCollider* ca = a.Collider;
		CCollider* cb = b.Collider;

		if (ca == nullptr || cb == nullptr)
			return false;

		Vec3 normal{};
		float penetration = 0.f;
		bool hit = false;

		const auto ta = ca->GetColliderType();
		const auto tb = cb->GetColliderType();

		if (ta == ECollider::E_Sphere && tb == ECollider::E_Sphere) {
			hit = CheckSphereSphere(
				static_cast<CSphereCollider*>(ca),
				static_cast<CSphereCollider*>(cb),
				normal, penetration);
		}
		else if (ta == ECollider::E_Sphere && tb == ECollider::E_Box) {
			hit = CheckSphereBox(
				static_cast<CSphereCollider*>(ca),
				static_cast<CBoxCollider*>(cb),
				normal, penetration);
		}
		else if (ta == ECollider::E_Box && tb == ECollider::E_Sphere) {
			hit = CheckSphereBox(
				static_cast<CSphereCollider*>(cb),
				static_cast<CBoxCollider*>(ca),
				normal, penetration);

			if (hit)
				normal = -normal; // A -> B 로 맞추기
		}
		else if (ta == ECollider::E_Box && tb == ECollider::E_Box) {
			hit = CheckBoxBox(
				static_cast<CBoxCollider*>(ca),
				static_cast<CBoxCollider*>(cb),
				normal, penetration);
		}

		if (!hit)
			return false;

		outContact.A = &a;
		outContact.B = &b;
		outContact.Normal = normal;
		outContact.Penetration = penetration;
		return true;
	}

	static Vec3 ComputeContactPoint(
		const PhysicsBodyEntry& a
		, const PhysicsBodyEntry& b
		, const Vec3& normal)
	{
		CCollider* ca = a.Collider;
		CCollider* cb = b.Collider;

		const auto ta = ca->GetColliderType();
		const auto tb = cb->GetColliderType();

		// ===== Sphere - Sphere =====
		if (ta == ECollider::E_Sphere && tb == ECollider::E_Sphere) {
			const Vec3 pa = GetColliderWorldPosition(ca);
			const float ra = GetSphereScaledRadius(static_cast<CSphereCollider*>(ca));

			return pa + normal * ra;
		}

		// ===== Sphere - Box =====
		if (ta == ECollider::E_Sphere && tb == ECollider::E_Box) {
			const Vec3 center = GetColliderWorldPosition(ca);
			const OBB obb = GetOBB(static_cast<CBoxCollider*>(cb));
			return ClosestPointOnOBB(obb, center);
		}

		if (ta == ECollider::E_Box && tb == ECollider::E_Sphere) {
			const Vec3 center = GetColliderWorldPosition(cb);
			const OBB obb = GetOBB(static_cast<CBoxCollider*>(ca));
			return ClosestPointOnOBB(obb, center);
		}

		// ===== Capsule - Sphere =====
		if (ta == ECollider::E_Capsule && tb == ECollider::E_Sphere) {
			const Segment seg = GetCapsuleSegment(static_cast<CCapsuleCollider*>(ca));
			const Vec3 center = GetColliderWorldPosition(cb);
			return ClosestPointOnSegment(center, seg.A, seg.B);
		}

		if (ta == ECollider::E_Sphere && tb == ECollider::E_Capsule) {
			const Segment seg = GetCapsuleSegment(static_cast<CCapsuleCollider*>(cb));
			const Vec3 center = GetColliderWorldPosition(ca);
			return ClosestPointOnSegment(center, seg.A, seg.B);
		}

		// ===== Capsule - Box =====
		if (ta == ECollider::E_Capsule && tb == ECollider::E_Box) {
			const Segment seg = GetCapsuleSegment(static_cast<CCapsuleCollider*>(ca));
			const OBB obb = GetOBB(static_cast<CBoxCollider*>(cb));
			const Vec3 mid = (seg.A + seg.B) * 0.5f;
			return ClosestPointOnOBB(obb, mid);
		}

		if (ta == ECollider::E_Box && tb == ECollider::E_Capsule) {
			const Segment seg = GetCapsuleSegment(static_cast<CCapsuleCollider*>(cb));
			const OBB obb = GetOBB(static_cast<CBoxCollider*>(ca));
			const Vec3 mid = (seg.A + seg.B) * 0.5f;
			return ClosestPointOnOBB(obb, mid);
		}

		// ===== Box - Box (임시) =====
		if (ta == ECollider::E_Box && tb == ECollider::E_Box) {
			const OBB obbA = GetOBB(static_cast<CBoxCollider*>(ca));
			const OBB obbB = GetOBB(static_cast<CBoxCollider*>(cb));

			auto SupportPoint = [](const OBB& obb, const Vec3& dir) -> Vec3 {
				return obb.Center
					+ obb.Axis[0] * ((obb.Axis[0].Dot(dir) >= 0.f) ? obb.HalfExtents.x : -obb.HalfExtents.x)
					+ obb.Axis[1] * ((obb.Axis[1].Dot(dir) >= 0.f) ? obb.HalfExtents.y : -obb.HalfExtents.y)
					+ obb.Axis[2] * ((obb.Axis[2].Dot(dir) >= 0.f) ? obb.HalfExtents.z : -obb.HalfExtents.z);
				};

			const Vec3 pA = SupportPoint(obbA, normal);		// A에서 B 방향 면
			const Vec3 pB = SupportPoint(obbB, -normal);	// B에서 A 방향 면
			return (pA + pB) * 0.5f;
		}

		return Vec3(0.f);
	}

	static void UpdateBodyInertiaFromCollider(PhysicsBodyEntry& body) {
		CRigidbody* rb = body.Rigidbody;
		CCollider* col = body.Collider;

		if (rb == nullptr || col == nullptr)
			return;

		if (!rb->IsDynamic()) {
			rb->SetLocalInertia(Vec3(0.f, 0.f, 0.f));
			rb->SetLocalInvInertia(Vec3(0.f, 0.f, 0.f));
			rb->ClearInertiaDirty();
			return;
		}

		const float mass = rb->GetMass();

		switch (col->GetColliderType()) {
		case ECollider::E_Box:
		{
			auto* box = static_cast<CBoxCollider*>(col);
			const Vec3 he = GetBoxScaledHalfExtents(box);
			rb->SetBoxInertia(mass, he * 2.f);
			break;
		}
		case ECollider::E_Sphere:
		{
			auto* sphere = static_cast<CSphereCollider*>(col);
			const float r = GetSphereScaledRadius(sphere);
			rb->SetSphereInertia(mass, r);
			break;
		}
		case ECollider::E_Capsule:
		{
			auto* capsule = static_cast<CCapsuleCollider*>(col);
			const float r = GetCapsuleScaledRadius(capsule);
			const float hs = GetCapsuleScaledHalfSegment(capsule);
			rb->SetCapsuleInertiaApprox(mass, r, hs);
			break;
		}
		default:
			rb->SetLocalInertia(Vec3(0.f, 0.f, 0.f));
			rb->SetLocalInvInertia(Vec3(0.f, 0.f, 0.f));
			break;
		}

		rb->ClearInertiaDirty();
	}
}

PhysicsManager::PhysicsManager()
	: mRigidbodies{}
	, mColliders{}
	, mBodies{}
	, mAccumulatedTime{}
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

bool PhysicsManager::IsRigidbodyRegistered(CRigidbody* rigidbody) const {
	if (rigidbody == nullptr) return false;

	auto iter = std::find(mRigidbodies.begin(), mRigidbodies.end(), rigidbody);
	return iter != mRigidbodies.end();
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

bool PhysicsManager::IsColliderRegistered(CCollider* collider) const {
	if (collider == nullptr) return false;

	auto iter = std::find(mColliders.begin(), mColliders.end(), collider);
	return iter != mColliders.end();
}

void PhysicsManager::Clear() {
	mRigidbodies.clear();
	mColliders.clear();
	mBodies.clear();

	mAccumulatedTime = 0.f;
}

void PhysicsManager::StepSimulation(float dt) {
	mCollidedColliders.clear();

	BuildBodyEntries();
	IntegrateBodies(dt);

	std::vector<Contact> contacts;
	contacts.reserve(mBodies.size());

	GenerateContacts(contacts);
	ResolvePenetrations(contacts);
	ResolveImpulses(contacts, dt);
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

	for (PhysicsBodyEntry& body : mBodies) {
		if (body.Rigidbody == nullptr)
			continue;

		if (body.Rigidbody->IsInertiaDirty()) 
			UpdateBodyInertiaFromCollider(body);
	}
}

void PhysicsManager::IntegrateBodies(float dt) {
	for (CRigidbody* rb : mRigidbodies) {
		if (rb == nullptr) continue;
		if (!rb->IsDynamic()) continue;

		auto transform = rb->Transform();
		if (transform == nullptr) continue;

		const float invMass = rb->GetInvMass();
		if (invMass <= 0.f) continue;

		// =========================
		// Linear
		// =========================
		Vec3 velocity = rb->GetLinearVelocity();

		if (rb->GetUseGravity()) {
			velocity += mGravity * dt;
		}

		Vec3 force = rb->ConsumeForceAccum();
		velocity += force * invMass * dt;

		// damping
		const float linearDamping = std::max(0.f, 1.f - rb->GetLinearDamping() * dt);
		velocity *= linearDamping;

		rb->SetLinearVelocity(velocity);

		transform->AddRelativePosition(velocity * dt);

		// =========================
		// Angular
		// =========================
		Vec3 omega = rb->GetAngularVelocity();

		const float maxAngularSpeed = PITwo; // rad/s
		float omegaLen = omega.Length();
		if (omegaLen > maxAngularSpeed)
			omega *= (maxAngularSpeed / omegaLen);

		// torque → angular accel
		Vec3 torque = rb->ConsumeTorqueAccum();
		Vec3 invI = rb->GetLocalInvInertia();

		Vec3 angularAccel = Vec3(
			torque.x * invI.x,
			torque.y * invI.y,
			torque.z * invI.z
		);

		omega += angularAccel * dt;

		// angular damping
		const float angularDamping = std::max(0.f, 1.f - rb->GetAngularDamping() * dt);
		omega *= angularDamping;

		rb->SetAngularVelocity(omega);

		// =========================
		// Quaternion integration (핵심)
		// =========================
		Vec3 deltaRot = omega * dt;

		transform->AddRelativeRotation(Vec3(
			XMConvertToDegrees(deltaRot.x),
			XMConvertToDegrees(deltaRot.y),
			XMConvertToDegrees(deltaRot.z)
		));
	}
}

void PhysicsManager::SolveCollisions(float dt) {
	UNREFERENCED_PARAMETER(dt);

	std::vector<Contact> contacts;
	contacts.reserve(mBodies.size());

	const size_t count = mBodies.size();

	for (size_t i = 0; i < count; ++i) {
		for (size_t j = i + 1; j < count; ++j) {
			PhysicsBodyEntry& a = mBodies[i];
			PhysicsBodyEntry& b = mBodies[j];

			if (a.Collider == nullptr || b.Collider == nullptr)
				continue;

			if (a.Collider->IsTrigger() || b.Collider->IsTrigger())
				continue;

			const bool aDynamic = (a.Rigidbody != nullptr && a.Rigidbody->IsDynamic());
			const bool bDynamic = (b.Rigidbody != nullptr && b.Rigidbody->IsDynamic());

			if (!aDynamic && !bDynamic)
				continue;

			Contact contact{};
			if (!GenerateContact(a, b, contact))
				continue;

			contacts.push_back(contact);
		}
	}

	constexpr int kSolverIterations = 6;

	for (int iter = 0; iter < kSolverIterations; ++iter) {
		for (Contact& c : contacts) {
			PositionalCorrection(*c.A, *c.B, c.Normal, c.Penetration);
			ApplyCollisionImpulse(*c.A, *c.B, c.Normal);
			ApplyFrictionImpulse(*c.A, *c.B, c.Normal);
		}
	}
}

void PhysicsManager::SubmitDebugDraw() {
	for (CCollider* collider : mColliders) {
		if (collider == nullptr)
			continue;

		DebugColliderShape desc{};
		desc.Type = collider->GetColliderType();

		const Vec3 localScale = collider->GetScale();

		Mat4 trans = XMMatrixTranslation(
			collider->GetOffset().x,
			collider->GetOffset().y,
			collider->GetOffset().z);

		Mat4 scale = XMMatrixScaling(
			localScale.x,
			localScale.y,
			localScale.z);

		// owner world 안에 owner scale 포함되어 있다고 가정
		desc.World = scale * trans * collider->Transform()->GetWorldMatrix();

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

void PhysicsManager::GenerateContacts(std::vector<Contact>& outContacts) {
	const size_t count = mBodies.size();

	for (size_t i = 0; i < count; ++i) {
		for (size_t j = i + 1; j < count; ++j) {
			PhysicsBodyEntry& a = mBodies[i];
			PhysicsBodyEntry& b = mBodies[j];

			if (a.Collider == nullptr || b.Collider == nullptr)
				continue;

			const bool aDynamic = (a.Rigidbody && a.Rigidbody->IsDynamic());
			const bool bDynamic = (b.Rigidbody && b.Rigidbody->IsDynamic());

			if (!aDynamic && !bDynamic)
				continue;

			if (a.Collider->IsTrigger() || b.Collider->IsTrigger())
				continue;

			Vec3 normal{};
			float penetration = 0.f;
			bool hit = false;

			auto typeA = a.Collider->GetColliderType();
			auto typeB = b.Collider->GetColliderType();

			if (typeA == ECollider::E_Sphere && typeB == ECollider::E_Sphere) {
				hit = CheckSphereSphere(
					static_cast<CSphereCollider*>(a.Collider),
					static_cast<CSphereCollider*>(b.Collider),
					normal, penetration);
			}
			else if (typeA == ECollider::E_Sphere && typeB == ECollider::E_Box) {
				hit = CheckSphereBox(
					static_cast<CSphereCollider*>(a.Collider),
					static_cast<CBoxCollider*>(b.Collider),
					normal, penetration);

				// CheckSphereBox는 box -> sphere를 주므로
				// A->B(sphere->box)로 맞추려면 뒤집어야 함
				if (hit) normal = -normal;
			}
			else if (typeA == ECollider::E_Box && typeB == ECollider::E_Sphere) {
				hit = CheckSphereBox(
					static_cast<CSphereCollider*>(b.Collider),
					static_cast<CBoxCollider*>(a.Collider),
					normal, penetration);

				// 여기서는 이미 A->B(box->sphere) 방향이므로 뒤집지 말아야 함
			}
			else if (typeA == ECollider::E_Box && typeB == ECollider::E_Box) {
				hit = CheckBoxBox(
					static_cast<CBoxCollider*>(a.Collider),
					static_cast<CBoxCollider*>(b.Collider),
					normal, penetration);
			}
			else if (typeA == ECollider::E_Capsule && typeB == ECollider::E_Sphere) {
				hit = CheckCapsuleSphere(
					static_cast<CCapsuleCollider*>(a.Collider),
					static_cast<CSphereCollider*>(b.Collider),
					normal, penetration);
			}
			else if (typeA == ECollider::E_Sphere && typeB == ECollider::E_Capsule) {
				hit = CheckCapsuleSphere(
					static_cast<CCapsuleCollider*>(b.Collider),
					static_cast<CSphereCollider*>(a.Collider),
					normal, penetration);
				if (hit) normal = -normal;
			}
			else if (typeA == ECollider::E_Capsule && typeB == ECollider::E_Capsule) {
				hit = CheckCapsuleCapsule(
					static_cast<CCapsuleCollider*>(a.Collider),
					static_cast<CCapsuleCollider*>(b.Collider),
					normal, penetration);
			}
			else if (typeA == ECollider::E_Capsule && typeB == ECollider::E_Box) {
				hit = CheckCapsuleBox(
					static_cast<CCapsuleCollider*>(a.Collider),
					static_cast<CBoxCollider*>(b.Collider),
					normal, penetration);

				// CheckCapsuleBox는 box -> capsule 이므로
				// A->B(capsule->box)로 맞추려면 뒤집어야 함
				if (hit) normal = -normal;
			}
			else if (typeA == ECollider::E_Box && typeB == ECollider::E_Capsule) {
				hit = CheckCapsuleBox(
					static_cast<CCapsuleCollider*>(b.Collider),
					static_cast<CBoxCollider*>(a.Collider),
					normal, penetration);

				// 여기서는 이미 A->B(box->capsule) 방향이므로 뒤집지 않음
			}

			if (!hit) continue;

			Contact c{};
			c.A = &a;
			c.B = &b;
			c.Normal = normal;
			c.Penetration = penetration;
			c.Point = ComputeContactPoint(a, b, normal);

			outContacts.push_back(c);

			mCollidedColliders.insert(a.Collider);
			mCollidedColliders.insert(b.Collider);
		}
	}
}

void PhysicsManager::ResolvePenetrations(std::vector<Contact>& contacts) {
	constexpr float kSlop = 0.001f;
	constexpr float kPercent = 0.8f;

	for (Contact& c : contacts) {
		auto& a = *c.A;
		auto& b = *c.B;

		float invMassA = (a.Rigidbody && a.Rigidbody->IsDynamic()) ? a.Rigidbody->GetInvMass() : 0.f;
		float invMassB = (b.Rigidbody && b.Rigidbody->IsDynamic()) ? b.Rigidbody->GetInvMass() : 0.f;

		float invMassSum = invMassA + invMassB;
		if (invMassSum <= 0.f)
			continue;

		float correctionMag =
			std::max(c.Penetration - kSlop, 0.f) * kPercent / invMassSum;

		Vec3 correction = c.Normal * correctionMag;

		if (invMassA > 0.f) {
			if (auto* t = a.Collider->Transform())
				t->AddRelativePosition(-correction * invMassA);
		}

		if (invMassB > 0.f) {
			if (auto* t = b.Collider->Transform())
				t->AddRelativePosition(correction * invMassB);
		}
	}
}

void PhysicsManager::ResolveImpulses(std::vector<Contact>& contacts, float dt) {
	UNREFERENCED_PARAMETER(dt);

	constexpr int kIterations = 6;

	for (int iter = 0; iter < kIterations; ++iter) {
		for (Contact& c : contacts) {
			auto& a = *c.A;
			auto& b = *c.B;

			CRigidbody* rbA = a.Rigidbody;
			CRigidbody* rbB = b.Rigidbody;

			const float invMassA = GetBodyInvMass(a);
			const float invMassB = GetBodyInvMass(b);
			if (invMassA + invMassB <= 0.f)
				continue;

			const Vec3 centerA = GetBodyWorldCenter(a);
			const Vec3 centerB = GetBodyWorldCenter(b);
			const Vec3 ra = c.Point - centerA;
			const Vec3 rb = c.Point - centerB;

			Vec3 va = (rbA ? rbA->GetLinearVelocity() : Vec3(0.f))
				+ (rbA ? rbA->GetAngularVelocity().Cross(ra) : Vec3(0.f));
			Vec3 vb = (rbB ? rbB->GetLinearVelocity() : Vec3(0.f))
				+ (rbB ? rbB->GetAngularVelocity().Cross(rb) : Vec3(0.f));

			Vec3 rv = vb - va;
			const float velAlongNormal = rv.Dot(c.Normal);
			if (velAlongNormal > 0.f)
				continue;

			float restitution = 0.f;
			if (rbA && rbB)
				restitution = std::min(rbA->GetRestitution(), rbB->GetRestitution());
			else if (rbA)
				restitution = rbA->GetRestitution();
			else if (rbB)
				restitution = rbB->GetRestitution();

			if (fabsf(velAlongNormal) < 1.0f)
				restitution = 0.f;

			const float normalDenom = ComputeImpulseDenominator(rbA, rbB, invMassA, invMassB, ra, rb, c.Normal);
			if (normalDenom <= kEpsilon)
				continue;

			const float j = -(1.f + restitution) * velAlongNormal / normalDenom;
			const Vec3 normalImpulse = c.Normal * j;
			ApplyImpulseAtPoint(a, b, c.Point, normalImpulse);

			va = (rbA ? rbA->GetLinearVelocity() : Vec3(0.f))
				+ (rbA ? rbA->GetAngularVelocity().Cross(ra) : Vec3(0.f));
			vb = (rbB ? rbB->GetLinearVelocity() : Vec3(0.f))
				+ (rbB ? rbB->GetAngularVelocity().Cross(rb) : Vec3(0.f));
			rv = vb - va;

			Vec3 tangent = rv - c.Normal * rv.Dot(c.Normal);
			const float tangentLenSq = tangent.LengthSquared();
			if (tangentLenSq <= kEpsilon)
				continue;

			tangent /= sqrtf(tangentLenSq);

			const float tangentDenom = ComputeImpulseDenominator(rbA, rbB, invMassA, invMassB, ra, rb, tangent);
			if (tangentDenom <= kEpsilon)
				continue;

			float jt = -rv.Dot(tangent) / tangentDenom;

			float friction = 0.5f;
			if (rbA && rbB)
				friction = sqrtf(rbA->GetFriction() * rbB->GetFriction());
			else if (rbA)
				friction = rbA->GetFriction();
			else if (rbB)
				friction = rbB->GetFriction();

			const float maxFriction = j * friction * 0.5f;
			jt = std::clamp(jt, -maxFriction, maxFriction);

			const Vec3 frictionImpulse = tangent * jt;
			ApplyImpulseAtPoint(a, b, c.Point, frictionImpulse);
		}
	}
}
