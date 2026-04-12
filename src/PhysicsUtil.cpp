#include "pch.h"
#include "PhysicsUtil.hpp"

#include "GameObject.hpp"

#include "CTransform.hpp"
#include "CRigidbody.hpp"
#include "CCollider.hpp"
#include "CBoxCollider.hpp"
#include "CSphereCollider.hpp"
#include "CCapsuleCollider.hpp"

namespace {
	static Vec3 GetBestAlignedBodyAxis(const PhysicsBodyEntry& body, const Vec3& targetNormal) {
		if (body.Rigidbody == nullptr || body.Rigidbody->Transform() == nullptr)
			return Vec3(0.f, 1.f, 0.f);

		const Mat4 world = body.Rigidbody->Transform()->GetWorldMatrix();
		Vec3 axes[6] = {
			PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(1.f, 0.f, 0.f), world), Vec3(1.f, 0.f, 0.f)),
			PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(-1.f, 0.f, 0.f), world), Vec3(-1.f, 0.f, 0.f)),
			PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 1.f, 0.f), world), Vec3(0.f, 1.f, 0.f)),
			PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f,-1.f, 0.f), world), Vec3(0.f,-1.f, 0.f)),
			PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), world), Vec3(0.f, 0.f, 1.f)),
			PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 0.f,-1.f), world), Vec3(0.f, 0.f,-1.f)),
		};

		float bestDot = -FLT_MAX;
		Vec3 bestAxis(0.f, 1.f, 0.f);
		for (const Vec3& axis : axes) {
			const float d = axis.Dot(targetNormal);
			if (d > bestDot) {
				bestDot = d;
				bestAxis = axis;
			}
		}
		return bestAxis;
	}
}


Vec3 PhysicsUtil::GetColliderWorldPosition(const CCollider* collider) {
	auto transform = collider ? collider->GetOwner()->Transform() : nullptr;
	if (transform == nullptr)
		return collider ? collider->GetOffset() : Vec3(0.f);

	// 기존 RelativePosition 대신 월드 기준으로 맞추는 게 안전
	const Mat4 world = transform->GetWorldMatrix();
	return Vec3::Transform(collider->GetOffset(), world);
}

Vec3 PhysicsUtil::GetFinalColliderScale(const CCollider* collider) {
	const Vec3 local = collider->GetScale();

	auto t = collider->GetOwner()->Transform();
	if (t == nullptr)
		return local;

	const Vec3 world = t->GetWorldScale();

	return Vec3(
		local.x * world.x,
		local.y * world.y,
		local.z * world.z
	);
}

Vec3 PhysicsUtil::GetBoxScaledHalfExtents(const CBoxCollider* box) {
	const Vec3 s = GetFinalColliderScale(box);
	const Vec3 he = box->GetHalfExtents();
	return Vec3(he.x * s.x, he.y * s.y, he.z * s.z);
}

float PhysicsUtil::GetSphereScaledRadius(const CSphereCollider* sphere) {
	const Vec3 s = GetFinalColliderScale(sphere);
	const float maxScale = std::max({ s.x, s.y, s.z });
	return sphere->GetRadius() * maxScale;
}

float PhysicsUtil::GetCapsuleScaledRadius(const CCapsuleCollider* capsule) {
	const Vec3 s = GetFinalColliderScale(capsule);

	float axisScale = 1.f;

	switch (capsule->GetAxis()) {
	case ECapsuleAxis::E_XAxis: axisScale = std::max(s.y, s.z); break;
	case ECapsuleAxis::E_YAxis: axisScale = std::max(s.x, s.z); break;
	case ECapsuleAxis::E_ZAxis: axisScale = std::max(s.x, s.y); break;
	}

	return capsule->GetRadius() * axisScale;
}

float PhysicsUtil::GetComponent(const Vec3& v, int idx) {
	switch (idx) {
	case 0: return v.x;
	case 1: return v.y;
	default: return v.z;
	}
}

Vec3 PhysicsUtil::GetAxisByIndex(const OBB& box, int idx) { return box.Axis[idx]; }

OBB PhysicsUtil::GetOBB(const CBoxCollider* box) {
	OBB out{};

	out.Center = GetColliderWorldPosition(box);
	out.HalfExtents = GetBoxScaledHalfExtents(box);

	const Mat4 world = box->GetOwner()->Transform()->GetWorldMatrix();

	out.Axis[0] = NormalizeSafe(Vec3::TransformNormal(Vec3(1.f, 0.f, 0.f), world), Vec3(1.f, 0.f, 0.f));
	out.Axis[1] = NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 1.f, 0.f), world), Vec3(0.f, 1.f, 0.f));
	out.Axis[2] = NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), world), Vec3(0.f, 0.f, 1.f));

	return out;
}

Vec3 PhysicsUtil::GetBodyWorldCenter(const PhysicsBodyEntry& body) {
	if (body.Rigidbody != nullptr && body.Rigidbody->Transform() != nullptr) {
		const Mat4 world = body.Rigidbody->Transform()->GetWorldMatrix();
		return Vec3::Transform(Vec3(0.f, 0.f, 0.f), world);
	}

	if (body.Collider != nullptr)
		return GetColliderWorldPosition(body.Collider);

	return Vec3(0.f);
}

float PhysicsUtil::GetBodyInvMass(const PhysicsBodyEntry& body) {
	if (body.Rigidbody == nullptr) return 0.f;
	if (!body.Rigidbody->IsDynamic()) return 0.f;
	return body.Rigidbody->GetInvMass();
}

float PhysicsUtil::GetCapsuleScaledHalfSegment(const CCapsuleCollider* capsule) {
	const Vec3 s = GetFinalColliderScale(capsule);

	float axisScale = 1.f;

	switch (capsule->GetAxis()) {
	case ECapsuleAxis::E_XAxis: axisScale = s.x; break;
	case ECapsuleAxis::E_YAxis: axisScale = s.y; break;
	case ECapsuleAxis::E_ZAxis: axisScale = s.z; break;
	}

	return capsule->GetHalfSegment() * axisScale;
}

Segment PhysicsUtil::GetCapsuleSegment(const CCapsuleCollider* capsule) {
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

void PhysicsUtil::GetFaceBasis(const OBB& box, int axisIndex, int sign, FaceBasis& outFace) {
	outFace.Normal = box.Axis[axisIndex] * static_cast<float>(sign);
	outFace.Center = box.Center + box.Axis[axisIndex] * (GetComponent(box.HalfExtents, axisIndex) * static_cast<float>(sign));

	switch (axisIndex) {
	case 0: // X face
		outFace.U = box.Axis[1];
		outFace.V = box.Axis[2];
		outFace.ExtentU = box.HalfExtents.y;
		outFace.ExtentV = box.HalfExtents.z;
		break;
	case 1: // Y face
		outFace.U = box.Axis[0];
		outFace.V = box.Axis[2];
		outFace.ExtentU = box.HalfExtents.x;
		outFace.ExtentV = box.HalfExtents.z;
		break;
	default: // Z face
		outFace.U = box.Axis[0];
		outFace.V = box.Axis[1];
		outFace.ExtentU = box.HalfExtents.x;
		outFace.ExtentV = box.HalfExtents.y;
		break;
	}
}

void PhysicsUtil::GetFaceVertices(const OBB& box, int axisIndex, int sign, std::vector<Vec3>& outVerts) {
	FaceBasis face{};
	GetFaceBasis(box, axisIndex, sign, face);

	outVerts.clear();
	outVerts.reserve(4);

	outVerts.push_back(face.Center + face.U * face.ExtentU + face.V * face.ExtentV);
	outVerts.push_back(face.Center - face.U * face.ExtentU + face.V * face.ExtentV);
	outVerts.push_back(face.Center - face.U * face.ExtentU - face.V * face.ExtentV);
	outVerts.push_back(face.Center + face.U * face.ExtentU - face.V * face.ExtentV);
}

Vec3 PhysicsUtil::NormalizeSafe(const Vec3& v, const Vec3& fallback) {
	const float lenSq = v.LengthSquared();
	if (lenSq <= kEpsilon)
		return fallback;

	Vec3 out = v;
	out /= sqrtf(lenSq);
	return out;
}

Vec3 PhysicsUtil::AbsVec3(const Vec3& v) { return Vec3(fabsf(v.x), fabsf(v.y), fabsf(v.z)); }

float PhysicsUtil::Clamp01(float v) { return std::clamp(v, 0.f, 1.f); }

Vec3 PhysicsUtil::WorldToOBBLocalPoint(const OBB& box, const Vec3& worldPoint) {
	const Vec3 d = worldPoint - box.Center;
	return Vec3(
		d.Dot(box.Axis[0]),
		d.Dot(box.Axis[1]),
		d.Dot(box.Axis[2])
	);
}

Vec3 PhysicsUtil::OBBLocalToWorldPoint(const OBB& box, const Vec3& localPoint) {
	return box.Center
		+ box.Axis[0] * localPoint.x
		+ box.Axis[1] * localPoint.y
		+ box.Axis[2] * localPoint.z;
}

Vec3 PhysicsUtil::ClosestPointOnOBB(const OBB& box, const Vec3& worldPoint) {
	Vec3 local = WorldToOBBLocalPoint(box, worldPoint);

	local.x = std::clamp(local.x, -box.HalfExtents.x, box.HalfExtents.x);
	local.y = std::clamp(local.y, -box.HalfExtents.y, box.HalfExtents.y);
	local.z = std::clamp(local.z, -box.HalfExtents.z, box.HalfExtents.z);

	return OBBLocalToWorldPoint(box, local);
}

bool PhysicsUtil::CheckSphereSphere(
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

bool PhysicsUtil::CheckSphereBox(
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

bool PhysicsUtil::CheckBoxBox(
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

	// 교차축 9개: 충돌 여부 판정만 하고 normal 후보로는 쓰지 않음
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

			const float overlap = (ra + rb) - fabsf(dist);
			if (overlap <= 0.f)
				return false;
		}
	}

	outNormal = bestAxis;
	outPenetration = minOverlap;
	return true;
}

void PhysicsUtil::ClosestPtSegmentSegment(
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

Vec3 PhysicsUtil::ClosestPointOnSegment(const Vec3& p, const Vec3& a, const Vec3& b) {
	Vec3 ab = b - a;
	float abLenSq = ab.LengthSquared();
	if (abLenSq <= 0.000001f)
		return a;

	float t = (p - a).Dot(ab) / abLenSq;
	t = std::clamp(t, 0.f, 1.f);
	return a + ab * t;
}

Vec3 PhysicsUtil::MulInvInertiaWorld(const CRigidbody* rb, const Vec3& v) {
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

float PhysicsUtil::ComputeImpulseDenominator(
	CRigidbody* rbA, CRigidbody* rbB
	, float invMassA, float invMassB
	, const Vec3& ra, const Vec3& rb
	, const Vec3& dir) {
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

void PhysicsUtil::ApplyImpulseAtPoint(
	PhysicsBodyEntry& a
	, PhysicsBodyEntry& b
	, const Vec3& point
	, const Vec3& impulse) {
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

bool PhysicsUtil::CheckCapsuleSphere(
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

bool PhysicsUtil::CheckCapsuleCapsule(
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

bool PhysicsUtil::CheckCapsuleBox(
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

void PhysicsUtil::PositionalCorrection(
	PhysicsBodyEntry& a
	, PhysicsBodyEntry& b
	, const Vec3& normal
	, float penetration) {
	const float invMassA = GetBodyInvMass(a);
	const float invMassB = GetBodyInvMass(b);
	const float invMassSum = invMassA + invMassB;

	if (invMassSum <= 0.f)
		return;

	constexpr float kSlop = 0.003f;
	float percent = 0.35f;
	if (IsBoxBoxPair(a, b))
		percent = 0.10f;

	const float correctionMag =
		std::max(penetration - kSlop, 0.f) * percent / invMassSum;

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

float PhysicsUtil::ComputeRestitution(CRigidbody* a, CRigidbody* b, float velAlongNormal) {
	float e = 0.f;

	if (a != nullptr && b != nullptr)
		e = std::min(a->GetRestitution(), b->GetRestitution());
	else if (a != nullptr)
		e = a->GetRestitution();
	else if (b != nullptr)
		e = b->GetRestitution();

	// resting / landing 구간에서는 반발 제거
	if (fabsf(velAlongNormal) < 1.0f)
		e = 0.f;

	return e;
}

float PhysicsUtil::ComputeFriction(CRigidbody* a, CRigidbody* b) {
	if (a != nullptr && b != nullptr)
		return sqrtf(a->GetFriction() * b->GetFriction());
	else if (a != nullptr)
		return a->GetFriction();
	else if (b != nullptr)
		return b->GetFriction();

	return 0.f;
}

void PhysicsUtil::ClipPolygonAgainstPlane(
	std::vector<Vec3>& poly
	, const Vec3& planeN
	, const Vec3& planeOrigin
	, float planeLimit) {
	if (poly.empty())
		return;

	std::vector<Vec3> input = poly;
	poly.clear();

	auto Dist = [&](const Vec3& p) -> float {
		return (p - planeOrigin).Dot(planeN) - planeLimit;
		};

	const size_t count = input.size();
	for (size_t i = 0; i < count; ++i) {
		const Vec3& a = input[i];
		const Vec3& b = input[(i + 1) % count];

		const float da = Dist(a);
		const float db = Dist(b);

		const bool aInside = (da <= 0.f);
		const bool bInside = (db <= 0.f);

		if (aInside && bInside) {
			poly.push_back(b);
		}
		else if (aInside && !bInside) {
			const float t = da / (da - db);
			poly.push_back(a + (b - a) * t);
		}
		else if (!aInside && bInside) {
			const float t = da / (da - db);
			poly.push_back(a + (b - a) * t);
			poly.push_back(b);
		}
	}
}

int PhysicsUtil::FindBestAxis(const OBB& box, const Vec3& dir, float& outAbsDot) {
	int bestAxis = 0;
	outAbsDot = -FLT_MAX;

	for (int i = 0; i < 3; ++i) {
		const float d = fabsf(box.Axis[i].Dot(dir));
		if (d > outAbsDot) {
			outAbsDot = d;
			bestAxis = i;
		}
	}

	return bestAxis;
}

int PhysicsUtil::FindIncidentFaceAxis(const OBB& box, const Vec3& referenceNormal, int& outSign) {
	int bestAxis = 0;
	float bestDot = FLT_MAX;

	for (int i = 0; i < 3; ++i) {
		const float d = box.Axis[i].Dot(referenceNormal);
		const float dNeg = (-box.Axis[i]).Dot(referenceNormal);

		if (d < bestDot) {
			bestDot = d;
			bestAxis = i;
			outSign = +1;
		}
		if (dNeg < bestDot) {
			bestDot = dNeg;
			bestAxis = i;
			outSign = -1;
		}
	}

	return bestAxis;
}

void PhysicsUtil::ReduceContactPoints(std::vector<Contact>& contacts, size_t maxCount) {
	if (contacts.size() <= maxCount)
		return;

	// 1. 가장 깊은 점 하나 선택
	size_t deepest = 0;
	for (size_t i = 1; i < contacts.size(); ++i) {
		if (contacts[i].Penetration > contacts[deepest].Penetration)
			deepest = i;
	}

	std::vector<Contact> reduced;
	reduced.reserve(maxCount);
	reduced.push_back(contacts[deepest]);

	// 2. 나머지는 기존 선택들과 최대한 멀리 떨어진 점 우선
	while (reduced.size() < maxCount) {
		size_t bestIdx = SIZE_MAX;
		float bestScore = -FLT_MAX;

		for (size_t i = 0; i < contacts.size(); ++i) {
			bool alreadyUsed = false;
			for (const Contact& c : reduced) {
				if ((contacts[i].Point - c.Point).LengthSquared() < 1e-6f) {
					alreadyUsed = true;
					break;
				}
			}
			if (alreadyUsed)
				continue;

			float minDistSq = FLT_MAX;
			for (const Contact& c : reduced) {
				minDistSq = std::min(minDistSq, (contacts[i].Point - c.Point).LengthSquared());
			}

			// penetration도 약간 반영
			const float score = minDistSq + contacts[i].Penetration * 0.25f;
			if (score > bestScore) {
				bestScore = score;
				bestIdx = i;
			}
		}

		if (bestIdx == SIZE_MAX)
			break;

		reduced.push_back(contacts[bestIdx]);
	}

	contacts = std::move(reduced);
}

void PhysicsUtil::GenerateBoxBoxManifold(
	PhysicsBodyEntry& a
	, PhysicsBodyEntry& b
	, const Vec3& normalAB
	, float pairPenetration
	, std::vector<Contact>& outContacts) {
	CCollider* ca = a.Collider;
	CCollider* cb = b.Collider;

	const OBB obbA = GetOBB(static_cast<CBoxCollider*>(ca));
	const OBB obbB = GetOBB(static_cast<CBoxCollider*>(cb));

	float scoreA = 0.f;
	float scoreB = 0.f;
	const int refAxisA = FindBestAxis(obbA, normalAB, scoreA);
	const int refAxisB = FindBestAxis(obbB, normalAB, scoreB);

	const bool aIsReference = (scoreA >= scoreB);

	const OBB& refBox = aIsReference ? obbA : obbB;
	const OBB& incBox = aIsReference ? obbB : obbA;

	const Vec3 solverNormal = normalAB;
	const Vec3 refNormal = aIsReference ? solverNormal : -solverNormal;

	const int refAxis = aIsReference ? refAxisA : refAxisB;
	const int refSign = (refBox.Axis[refAxis].Dot(refNormal) >= 0.f) ? +1 : -1;

	FaceBasis refFace{};
	GetFaceBasis(refBox, refAxis, refSign, refFace);

	int incSign = +1;
	const int incAxis = FindIncidentFaceAxis(incBox, refFace.Normal, incSign);

	std::vector<Vec3> poly;
	GetFaceVertices(incBox, incAxis, incSign, poly);

	// reference face rectangle로 clip
	ClipPolygonAgainstPlane(poly, refFace.U, refFace.Center, refFace.ExtentU);
	ClipPolygonAgainstPlane(poly, -refFace.U, refFace.Center, refFace.ExtentU);
	ClipPolygonAgainstPlane(poly, refFace.V, refFace.Center, refFace.ExtentV);
	ClipPolygonAgainstPlane(poly, -refFace.V, refFace.Center, refFace.ExtentV);

	std::vector<Contact> manifold;
	manifold.reserve(poly.size());

	float deepestPen = 0.f;
	Vec3 deepestPoint = Vec3(0.f);

	for (const Vec3& p : poly) {
		// reference plane 기준 음수면 안쪽으로 들어온 것
		const float separation = (p - refFace.Center).Dot(refFace.Normal);
		const float pen = std::max(-separation, 0.f);

		// 실제로 거의 안 파고든 점은 제외
		if (pen <= 0.0005f)
			continue;

		// contact point는 plane 위 projection으로 고정
		const Vec3 projected = p - refFace.Normal * separation;

		Contact c{};
		c.A = &a;
		c.B = &b;
		c.Normal = solverNormal;
		c.Point = projected;
		c.Penetration = pen;

		manifold.push_back(c);

		if (pen > deepestPen) {
			deepestPen = pen;
			deepestPoint = projected;
		}
	}

	// 클리핑 결과가 빈약하면 fallback 하나만
	if (manifold.empty()) {
		Contact c{};
		c.A = &a;
		c.B = &b;
		c.Normal = solverNormal;

		const Vec3 pOnB = ClosestPointOnOBB(obbB, obbA.Center);
		const Vec3 pOnA = ClosestPointOnOBB(obbA, obbB.Center);

		c.Point = (pOnA + pOnB) * 0.5f;
		c.Penetration = std::max(pairPenetration, 0.f);

		manifold.push_back(c);
	}
	else {
		// ===== 핵심 수정 =====
		// 모서리/엣지 접촉처럼 접촉점이 적거나 좁게 몰린 경우엔
		// manifold를 부풀리지 말고 단일 contact로 축소한다.
		if (manifold.size() <= 2) {
			Contact best = manifold[0];
			for (size_t i = 1; i < manifold.size(); ++i) {
				if (manifold[i].Penetration > best.Penetration)
					best = manifold[i];
			}

			// point를 너무 한쪽 꼭짓점으로 몰지 않도록
			Vec3 avgPoint(0.f);
			for (const Contact& c : manifold)
				avgPoint += c.Point;
			avgPoint /= static_cast<float>(manifold.size());

			best.Point = avgPoint;

			// edge/corner에서는 penetration 바닥값 과증폭 금지
			best.Penetration = std::max(best.Penetration, pairPenetration * 0.5f);

			manifold.clear();
			manifold.push_back(best);
		}
		else {
			// 면 접촉일 때만 penetration floor 유지
			for (Contact& c : manifold) {
				c.Penetration = std::max(c.Penetration, deepestPen * 0.35f);
			}

			ReduceContactPoints(manifold, 4);
		}
	}

	for (Contact& c : manifold) {
		outContacts.push_back(c);
	}
}

void PhysicsUtil::BuildContactBasis(const Vec3& normal, Vec3& outT1, Vec3& outT2) {
	// normal과 덜 평행한 축을 골라 안정적인 basis 생성
	Vec3 ref = (fabsf(normal.y) < 0.9f) ? Vec3(0.f, 1.f, 0.f) : Vec3(1.f, 0.f, 0.f);

	outT1 = normal.Cross(ref);
	const float lenSq = outT1.LengthSquared();
	if (lenSq <= kEpsilon) {
		ref = Vec3(0.f, 0.f, 1.f);
		outT1 = normal.Cross(ref);
	}

	outT1.Normalize();
	outT2 = normal.Cross(outT1);
	outT2.Normalize();
}

void PhysicsUtil::ApplyFrictionImpulseAlong(
	PhysicsBodyEntry& a
	, PhysicsBodyEntry& b
	, const Contact& c
	, const Vec3& dir
	, float normalImpulseMagnitude) {
	CRigidbody* rbA = a.Rigidbody;
	CRigidbody* rbB = b.Rigidbody;

	const float invMassA = GetBodyInvMass(a);
	const float invMassB = GetBodyInvMass(b);

	if (invMassA + invMassB <= 0.f)
		return;

	const Vec3 ra = c.Point - GetBodyWorldCenter(a);
	const Vec3 rb = c.Point - GetBodyWorldCenter(b);

	const Vec3 va =
		(rbA ? rbA->GetLinearVelocity() : Vec3(0.f)) +
		(rbA ? rbA->GetAngularVelocity().Cross(ra) : Vec3(0.f));

	const Vec3 vb =
		(rbB ? rbB->GetLinearVelocity() : Vec3(0.f)) +
		(rbB ? rbB->GetAngularVelocity().Cross(rb) : Vec3(0.f));

	const Vec3 rv = vb - va;
	const float rel = rv.Dot(dir);

	const float denom = ComputeImpulseDenominator(rbA, rbB, invMassA, invMassB, ra, rb, dir);
	if (denom <= kEpsilon)
		return;

	float jt = -rel / denom;

	float mu = ComputeFriction(rbA, rbB);

	// manifold contact에서는 조금 더 강하게 잡아도 안정적
	mu *= 1.15f;

	const float maxJt = normalImpulseMagnitude * mu;
	jt = std::clamp(jt, -maxJt, maxJt);

	ApplyImpulseAtPoint(a, b, c.Point, dir * jt);
}

void PhysicsUtil::ResolveImpulse(
	PhysicsBodyEntry& a,
	PhysicsBodyEntry& b,
	const Contact& c,
	float restitutionScale) {
	const float jn = ApplyNormalImpulseAtContact(a, b, c, restitutionScale);
	if (jn <= kEpsilon)
		return;

	Vec3 t1{}, t2{};
	BuildContactBasis(c.Normal, t1, t2);

	ApplyFrictionImpulseAlong(a, b, c, t1, jn);
	ApplyFrictionImpulseAlong(a, b, c, t2, jn);
}

bool PhysicsUtil::GenerateContact(
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

Vec3 PhysicsUtil::ComputeContactPoint(
	const PhysicsBodyEntry& a
	, const PhysicsBodyEntry& b
	, const Vec3& normal) {
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

	// ===== Box - Box =====
	if (ta == ECollider::E_Box && tb == ECollider::E_Box) {
		const OBB obbA = GetOBB(static_cast<CBoxCollider*>(ca));
		const OBB obbB = GetOBB(static_cast<CBoxCollider*>(cb));

		// 각 박스 중심을 상대 박스 표면에 투영
		const Vec3 pOnB = ClosestPointOnOBB(obbB, obbA.Center);
		const Vec3 pOnA = ClosestPointOnOBB(obbA, obbB.Center);

		// 둘의 중간점을 접촉점으로 사용
		return (pOnA + pOnB) * 0.5f;
	}

	return Vec3(0.f);
}

void PhysicsUtil::UpdateBodyInertiaFromCollider(PhysicsBodyEntry& body) {
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

bool PhysicsUtil::IsRollingShape(const PhysicsBodyEntry& body) {
	if (body.Collider == nullptr)
		return false;

	const auto type = body.Collider->GetColliderType();
	return type == ECollider::E_Sphere || type == ECollider::E_Capsule;
}

float PhysicsUtil::GetRollingRadius(const PhysicsBodyEntry& body) {
	if (body.Collider == nullptr)
		return 0.f;

	switch (body.Collider->GetColliderType()) {
	case ECollider::E_Sphere:
		return GetSphereScaledRadius(static_cast<CSphereCollider*>(body.Collider));
	case ECollider::E_Capsule:
		return GetCapsuleScaledRadius(static_cast<CCapsuleCollider*>(body.Collider));
	default:
		return 0.f;
	}
}

void PhysicsUtil::StabilizeRollingAngularVelocity(std::vector<Contact>& contacts) {
	struct ContactAccum {
		Vec3 NormalSum = Vec3(0.f);
		int Count = 0;
	};

	std::unordered_map<CRigidbody*, ContactAccum> accumMap;

	for (Contact& c : contacts) {
		if (c.A && c.A->Rigidbody && IsRollingShape(*c.A)) {
			accumMap[c.A->Rigidbody].NormalSum += -c.Normal;
			accumMap[c.A->Rigidbody].Count++;
		}

		if (c.B && c.B->Rigidbody && IsRollingShape(*c.B)) {
			accumMap[c.B->Rigidbody].NormalSum += c.Normal;
			accumMap[c.B->Rigidbody].Count++;
		}
	}

	for (auto& [rb, acc] : accumMap) {
		if (rb == nullptr || !rb->IsDynamic() || acc.Count <= 0)
			continue;

		CCollider* col = rb->GetOwner() ? rb->GetOwner()->GetColliderComponent().Get() : nullptr;
		if (col == nullptr)
			continue;

		PhysicsBodyEntry body{};
		body.Rigidbody = rb;
		body.Collider = col;

		const float radius = GetRollingRadius(body);
		if (radius <= 0.0001f)
			continue;

		Vec3 n = NormalizeSafe(acc.NormalSum / static_cast<float>(acc.Count));
		Vec3 v = rb->GetLinearVelocity();

		// 접촉면 접선 속도
		Vec3 vt = v - n * v.Dot(n);
		const float vtLenSq = vt.LengthSquared();

		Vec3 omega = rb->GetAngularVelocity();

		// 법선축 스핀 제거
		omega -= n * omega.Dot(n);

		if (vtLenSq <= 0.03f * 0.03f) {
			// 약한 경사 / 거의 정지 상태에서는 각속도 강하게 감쇠
			omega *= 0.7f;
			rb->SetAngularVelocity(omega);
			continue;
		}

		// 굴림 목표 각속도
		Vec3 omegaTarget = (n.Cross(vt)) / radius;

		// 여유 상한
		const float maxOmega = (sqrtf(vtLenSq) / radius) * 1.35f;
		const float targetLen = omegaTarget.Length();
		if (targetLen > maxOmega && targetLen > 0.000001f) {
			omegaTarget *= (maxOmega / targetLen);
		}

		// 현재 각속도를 목표 쪽으로 끌어감
		// 0.2 ~ 0.35 정도 사이에서 조절 추천
		const float follow = 0.18f;
		omega = omega + (omegaTarget - omega) * follow;

		rb->SetAngularVelocity(omega);
	}
}

PairKey PhysicsUtil::MakePairKey(const Contact& c) {
	const CCollider* a = c.A->Collider;
	const CCollider* b = c.B->Collider;
	if (a < b) return { a, b };
	return { b, a };
}

Vec3 PhysicsUtil::GetVelocityAtPoint(const PhysicsBodyEntry& body, const Vec3& point) {
	CRigidbody* rb = body.Rigidbody;
	if (rb == nullptr)
		return Vec3(0.f);

	const Vec3 center = GetBodyWorldCenter(body);
	const Vec3 r = point - center;

	return rb->GetLinearVelocity() + rb->GetAngularVelocity().Cross(r);
}

float PhysicsUtil::ApplyNormalImpulseAtContact(
	PhysicsBodyEntry& a,
	PhysicsBodyEntry& b,
	const Contact& c,
	float restitutionScale) {
	CRigidbody* rbA = a.Rigidbody;
	CRigidbody* rbB = b.Rigidbody;

	const float invMassA = GetBodyInvMass(a);
	const float invMassB = GetBodyInvMass(b);

	if (invMassA + invMassB <= 0.f)
		return 0.f;

	const Vec3 ra = c.Point - GetBodyWorldCenter(a);
	const Vec3 rb = c.Point - GetBodyWorldCenter(b);

	const Vec3 va = GetVelocityAtPoint(a, c.Point);
	const Vec3 vb = GetVelocityAtPoint(b, c.Point);
	const Vec3 rv = vb - va;

	const float vn = rv.Dot(c.Normal);
	if (vn > 0.f)
		return 0.f;

	const float denom = ComputeImpulseDenominator(
		rbA, rbB,
		invMassA, invMassB,
		ra, rb,
		c.Normal);

	if (denom <= kEpsilon)
		return 0.f;

	float restitution = ComputeRestitution(rbA, rbB, vn) * restitutionScale;

	if (IsBoxBoxPair(a, b) && fabsf(vn) < 1.25f)
		restitution = 0.f;

	float jn = -(1.f + restitution) * vn / denom;
	jn = std::max(jn, 0.f);

	ApplyImpulseAtPoint(a, b, c.Point, c.Normal * jn);
	return jn;
}

void PhysicsUtil::ApplyPairFrictionImpulse(
	PhysicsBodyEntry& a
	, PhysicsBodyEntry& b
	, const Vec3& point
	, const Vec3& normal
	, float normalImpulseSum) {
	CRigidbody* rbA = a.Rigidbody;
	CRigidbody* rbB = b.Rigidbody;

	const float invMassA = GetBodyInvMass(a);
	const float invMassB = GetBodyInvMass(b);

	if (invMassA + invMassB <= 0.f)
		return;
	if (normalImpulseSum <= kEpsilon)
		return;

	const Vec3 va = GetVelocityAtPoint(a, point);
	const Vec3 vb = GetVelocityAtPoint(b, point);
	const Vec3 rv = vb - va;

	Vec3 t1{}, t2{};
	BuildContactBasis(normal, t1, t2);

	const Vec3 ra = point - GetBodyWorldCenter(a);
	const Vec3 rb = point - GetBodyWorldCenter(b);

	const float denom1 = ComputeImpulseDenominator(rbA, rbB, invMassA, invMassB, ra, rb, t1);
	const float denom2 = ComputeImpulseDenominator(rbA, rbB, invMassA, invMassB, ra, rb, t2);

	if (denom1 <= kEpsilon || denom2 <= kEpsilon)
		return;

	float mu = ComputeFriction(rbA, rbB);

	// static friction을 조금 더 강하게
	const float muStatic = mu * 1.35f;
	const float muDynamic = mu * 0.95f;

	const float vt1 = rv.Dot(t1);
	const float vt2 = rv.Dot(t2);

	float jt1 = -vt1 / denom1;
	float jt2 = -vt2 / denom2;

	const float jtLen = sqrtf(jt1 * jt1 + jt2 * jt2);
	const float maxStatic = normalImpulseSum * muStatic;

	if (jtLen <= maxStatic) {
		ApplyImpulseAtPoint(a, b, point, t1 * jt1);
		ApplyImpulseAtPoint(a, b, point, t2 * jt2);
	}
	else {
		const float vtLen = sqrtf(vt1 * vt1 + vt2 * vt2);
		if (vtLen <= kEpsilon)
			return;

		const Vec3 tangentDir = (t1 * vt1 + t2 * vt2) / vtLen;
		const float denom = ComputeImpulseDenominator(rbA, rbB, invMassA, invMassB, ra, rb, tangentDir);
		if (denom <= kEpsilon)
			return;

		float jt = -vtLen / denom;
		const float maxDynamic = normalImpulseSum * muDynamic;
		jt = std::clamp(jt, -maxDynamic, maxDynamic);

		ApplyImpulseAtPoint(a, b, point, tangentDir * jt);
	}
}

bool PhysicsUtil::IsBoxBoxPair(const PhysicsBodyEntry& a, const PhysicsBodyEntry& b) {
	if (a.Collider == nullptr || b.Collider == nullptr)
		return false;

	return a.Collider->GetColliderType() == ECollider::E_Box &&
		b.Collider->GetColliderType() == ECollider::E_Box;
}

Vec3 PhysicsUtil::ComputeAveragePoint(const std::vector<Contact*>& manifold) {
	Vec3 p(0.f);
	if (manifold.empty())
		return p;

	for (const Contact* c : manifold) {
		p += c->Point;
	}
	return p / static_cast<float>(manifold.size());
}

Vec3 PhysicsUtil::ComputeAverageNormal(const std::vector<Contact*>& manifold) {
	Vec3 n(0.f);
	if (manifold.empty())
		return Vec3(0.f, 1.f, 0.f);

	for (const Contact* c : manifold) {
		n += c->Normal;
	}
	return NormalizeSafe(n / static_cast<float>(manifold.size()));
}

void PhysicsUtil::RemoveRelativeTangentialVelocityAtPoint(
	PhysicsBodyEntry& a
	, PhysicsBodyEntry& b
	, const Vec3& point
	, const Vec3& normal
	, float maxImpulseScale) {
	CRigidbody* rbA = a.Rigidbody;
	CRigidbody* rbB = b.Rigidbody;

	const float invMassA = GetBodyInvMass(a);
	const float invMassB = GetBodyInvMass(b);

	if (invMassA + invMassB <= 0.f)
		return;

	const Vec3 va = GetVelocityAtPoint(a, point);
	const Vec3 vb = GetVelocityAtPoint(b, point);
	const Vec3 rv = vb - va;

	const float vn = rv.Dot(normal);
	Vec3 vt = rv - normal * vn;
	const float vtLen = vt.Length();

	if (vtLen <= 1e-4f)
		return;

	const Vec3 tangent = vt / vtLen;

	const Vec3 ra = point - GetBodyWorldCenter(a);
	const Vec3 rb = point - GetBodyWorldCenter(b);

	const float denom = ComputeImpulseDenominator(
		rbA, rbB,
		invMassA, invMassB,
		ra, rb,
		tangent);

	if (denom <= kEpsilon)
		return;

	// tangent relative velocity를 거의 제거하는 impulse
	float jt = -vtLen / denom;

	// 과보정 방지
	jt = std::clamp(jt, -maxImpulseScale, maxImpulseScale);

	ApplyImpulseAtPoint(a, b, point, tangent * jt);
}

void PhysicsUtil::StabilizeRestingBoxContacts(std::vector<Contact>& contacts) {
	std::unordered_map<PairKey, std::vector<Contact*>, PairKeyHash> manifolds;
	manifolds.reserve(contacts.size());

	for (Contact& c : contacts) {
		manifolds[MakePairKey(c)].push_back(&c);
	}

	const Vec3 worldUp(0.f, 1.f, 0.f);

	for (auto& [key, manifold] : manifolds) {
		if (manifold.empty())
			continue;

		PhysicsBodyEntry& a = *manifold[0]->A;
		PhysicsBodyEntry& b = *manifold[0]->B;

		if (!IsBoxBoxPair(a, b))
			continue;

		Vec3 avgPoint = ComputeAveragePoint(manifold);
		Vec3 avgNormal = ComputeAverageNormal(manifold);

		PhysicsBodyEntry* dynamicBody = nullptr;
		PhysicsBodyEntry* supportBody = nullptr;
		if (!PickDynamicVsSupport(a, b, dynamicBody, supportBody, avgNormal))
			continue;

		if (dynamicBody == nullptr || dynamicBody->Rigidbody == nullptr)
			continue;

		if (avgNormal.Dot(worldUp) < 0.95f)
			continue;
		if (manifold.size() < 3)
			continue;

		const Vec3 supportNormal = NormalizeSafe(avgNormal, worldUp);
		const Vec3 supportFaceAxis = GetBestAlignedBodyAxis(*dynamicBody, supportNormal);
		if (supportFaceAxis.Dot(supportNormal) < 0.95f)
			continue;

		CRigidbody* dynRb = dynamicBody->Rigidbody;
		if (dynRb->GetLinearVelocity().Length() > 0.35f)
			continue;
		if (dynRb->GetAngularVelocity().Length() > 1.0f)
			continue;

		const Vec3 vDyn = GetVelocityAtPoint(*dynamicBody, avgPoint);
		const Vec3 vSup = GetVelocityAtPoint(*supportBody, avgPoint);
		const Vec3 rv = vDyn - vSup;

		const float vn = rv.Dot(supportNormal);
		Vec3 vt = rv - supportNormal * vn;
		const float vtLen = vt.Length();

		float avgPenetration = 0.f;
		for (const Contact* c : manifold)
			avgPenetration += c->Penetration;
		avgPenetration /= static_cast<float>(manifold.size());

		const Vec3 dynCenter = GetBodyWorldCenter(*dynamicBody);
		const float supportHeight = (dynCenter - avgPoint).Dot(supportNormal);
		if (supportHeight < -0.02f)
			continue;

		if (fabsf(vn) < 0.10f && vtLen < 0.25f && avgPenetration < 0.08f) {
			Vec3 linear = dynRb->GetLinearVelocity();
			const float linN = linear.Dot(supportNormal);
			if (linN < 0.f)
				linear -= supportNormal * linN;

			Vec3 linT = linear - supportNormal * linear.Dot(supportNormal);
			const float linTLen = linT.Length();
			if (linTLen < 0.08f) {
				linear -= linT;
			}
			else {
				linear -= linT * 0.20f;
			}

			const float postN = linear.Dot(supportNormal);
			if (fabsf(postN) < 0.03f)
				linear -= supportNormal * postN;
			dynRb->SetLinearVelocity(linear);

			Vec3 w = dynRb->GetAngularVelocity();
			Vec3 wN = supportNormal * w.Dot(supportNormal);
			Vec3 wT = w - wN;
			wT *= 0.93f;
			wN *= 0.97f;
			if (wT.Length() < 0.04f)
				wT = Vec3(0.f);
			if (wN.Length() < 0.03f)
				wN = Vec3(0.f);
			dynRb->SetAngularVelocity(wN + wT);
		}
	}
}

void PhysicsUtil::StabilizeBoxBoxLanding(std::vector<Contact>& contacts) {
	std::unordered_map<PairKey, std::vector<Contact*>, PairKeyHash> manifolds;
	manifolds.reserve(contacts.size());

	for (Contact& c : contacts) {
		manifolds[MakePairKey(c)].push_back(&c);
	}

	const Vec3 worldUp(0.f, 1.f, 0.f);

	for (auto& [key, manifold] : manifolds) {
		if (manifold.empty())
			continue;

		PhysicsBodyEntry& a = *manifold[0]->A;
		PhysicsBodyEntry& b = *manifold[0]->B;

		if (a.Collider == nullptr || b.Collider == nullptr)
			continue;
		if (a.Collider->GetColliderType() != ECollider::E_Box ||
			b.Collider->GetColliderType() != ECollider::E_Box)
			continue;

		Vec3 avgNormal(0.f);
		Vec3 avgPoint(0.f);
		for (Contact* cp : manifold) {
			avgNormal += cp->Normal;
			avgPoint += cp->Point;
		}
		avgNormal = NormalizeSafe(avgNormal / static_cast<float>(manifold.size()));
		avgPoint /= static_cast<float>(manifold.size());

		if (avgNormal.Dot(worldUp) < 0.95f)
			continue;

		PhysicsBodyEntry* dynamicBody = nullptr;
		PhysicsBodyEntry* supportBody = nullptr;
		Vec3 supportNormal = avgNormal;
		if (!PickDynamicVsSupport(a, b, dynamicBody, supportBody, supportNormal))
			continue;

		const Vec3 supportFaceAxis = GetBestAlignedBodyAxis(*dynamicBody, supportNormal);
		if (supportFaceAxis.Dot(supportNormal) < 0.90f)
			continue;

		const Vec3 va = GetVelocityAtPoint(a, avgPoint);
		const Vec3 vb = GetVelocityAtPoint(b, avgPoint);
		const Vec3 rv = vb - va;

		const float vn = rv.Dot(avgNormal);
		Vec3 vt = rv - avgNormal * vn;
		if (fabsf(vn) < 0.25f && vt.Length() < 0.25f) {
			CRigidbody* rbA = a.Rigidbody;
			CRigidbody* rbB = b.Rigidbody;

			if (rbA && rbA->IsDynamic()) {
				Vec3 w = rbA->GetAngularVelocity();
				Vec3 wN = avgNormal * w.Dot(avgNormal);
				Vec3 wT = w - wN;
				rbA->SetAngularVelocity(wN + wT * 0.98f);
			}

			if (rbB && rbB->IsDynamic()) {
				Vec3 w = rbB->GetAngularVelocity();
				Vec3 wN = avgNormal * w.Dot(avgNormal);
				Vec3 wT = w - wN;
				rbB->SetAngularVelocity(wN + wT * 0.98f);
			}
		}
	}
}

bool PhysicsUtil::IsMostlyStaticSupport(
	const PhysicsBodyEntry& a
	, const PhysicsBodyEntry& b
	, PhysicsBodyEntry*& dynBody
	, PhysicsBodyEntry*& supportBody) {
	const bool aDyn = (a.Rigidbody && a.Rigidbody->IsDynamic() && a.Rigidbody->GetInvMass() > 0.f);
	const bool bDyn = (b.Rigidbody && b.Rigidbody->IsDynamic() && b.Rigidbody->GetInvMass() > 0.f);

	if (aDyn && !bDyn) {
		dynBody = const_cast<PhysicsBodyEntry*>(&a);
		supportBody = const_cast<PhysicsBodyEntry*>(&b);
		return true;
	}
	if (!aDyn && bDyn) {
		dynBody = const_cast<PhysicsBodyEntry*>(&b);
		supportBody = const_cast<PhysicsBodyEntry*>(&a);
		return true;
	}

	return false;
}

bool PhysicsUtil::IsDynamicBody(const PhysicsBodyEntry& body) {
	return body.Rigidbody != nullptr &&
		body.Rigidbody->IsDynamic() &&
		body.Rigidbody->GetInvMass() > 0.f;
}

bool PhysicsUtil::IsStaticBody(const PhysicsBodyEntry& body) {
	return body.Rigidbody == nullptr ||
		!body.Rigidbody->IsDynamic() ||
		body.Rigidbody->GetInvMass() <= 0.f;
}

bool PhysicsUtil::PickDynamicVsSupport(
	PhysicsBodyEntry& a
	, PhysicsBodyEntry& b
	, PhysicsBodyEntry*& outDynamic
	, PhysicsBodyEntry*& outSupport
	, Vec3& inOutNormal) {
	const bool aDyn = IsDynamicBody(a);
	const bool bDyn = IsDynamicBody(b);

	// 동적 1개 + 정적/준정적 1개 조합만 처리
	if (aDyn && IsStaticBody(b)) {
		outDynamic = &a;
		outSupport = &b;

		// normal이 support -> dynamic 방향이 되도록 정렬
		const Vec3 dynCenter = PhysicsUtil::GetBodyWorldCenter(a);
		const Vec3 supCenter = PhysicsUtil::GetBodyWorldCenter(b);
		if ((dynCenter - supCenter).Dot(inOutNormal) < 0.f)
			inOutNormal = -inOutNormal;

		return true;
	}

	if (bDyn && IsStaticBody(a)) {
		outDynamic = &b;
		outSupport = &a;

		const Vec3 dynCenter = PhysicsUtil::GetBodyWorldCenter(b);
		const Vec3 supCenter = PhysicsUtil::GetBodyWorldCenter(a);
		if ((dynCenter - supCenter).Dot(inOutNormal) < 0.f)
			inOutNormal = -inOutNormal;

		return true;
	}

	return false;
}

Vec3 PhysicsUtil::GetBodyUpAxis(const PhysicsBodyEntry& body) {
	if (body.Rigidbody == nullptr || body.Rigidbody->Transform() == nullptr)
		return Vec3(0.f, 1.f, 0.f);

	const Mat4 world = body.Rigidbody->Transform()->GetWorldMatrix();
	return PhysicsUtil::NormalizeSafe(
		Vec3::TransformNormal(Vec3(0.f, 1.f, 0.f), world),
		Vec3(0.f, 1.f, 0.f)
	);
}

Vec3 PhysicsUtil::GetBestAlignedBodyAxis(const PhysicsBodyEntry& body, const Vec3& targetNormal) {
	if (body.Rigidbody == nullptr || body.Rigidbody->Transform() == nullptr)
		return Vec3(0.f, 1.f, 0.f);

	const Mat4 world = body.Rigidbody->Transform()->GetWorldMatrix();

	Vec3 axes[6] = {
		PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(1.f, 0.f, 0.f), world), Vec3(1.f, 0.f, 0.f)),
		PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(-1.f, 0.f, 0.f), world), Vec3(-1.f, 0.f, 0.f)),
		PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 1.f, 0.f), world), Vec3(0.f, 1.f, 0.f)),
		PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f,-1.f, 0.f), world), Vec3(0.f,-1.f, 0.f)),
		PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), world), Vec3(0.f, 0.f, 1.f)),
		PhysicsUtil::NormalizeSafe(Vec3::TransformNormal(Vec3(0.f, 0.f,-1.f), world), Vec3(0.f, 0.f,-1.f)),
	};

	float bestDot = -FLT_MAX;
	Vec3 bestAxis = Vec3(0.f, 1.f, 0.f);

	for (const Vec3& axis : axes) {
		const float d = axis.Dot(targetNormal);
		if (d > bestDot) {
			bestDot = d;
			bestAxis = axis;
		}
	}

	return bestAxis;
}