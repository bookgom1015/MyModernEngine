#pragma once

class CRigidbody;
class CCollider;
class CSphereCollider;
class CBoxCollider;
class CCapsuleCollider;

struct Segment {
	Vec3 A;
	Vec3 B;
};

struct OBB {
	Vec3 Center;
	Vec3 Axis[3];      // 정규화된 월드 축
	Vec3 HalfExtents;  // 각 축 방향 half extent
};

struct FaceBasis {
	Vec3 Center;
	Vec3 Normal;
	Vec3 U;
	Vec3 V;
	float ExtentU = 0.f;
	float ExtentV = 0.f;
};

struct PhysicsBodyEntry {
	CRigidbody* Rigidbody = nullptr;
	CCollider* Collider = nullptr;
};

struct Contact {
	PhysicsBodyEntry* A = nullptr;
	PhysicsBodyEntry* B = nullptr;

	Vec3 Normal = Vec3(0.f, 1.f, 0.f); // A -> B
	float Penetration = 0.f;

	Vec3 Point = Vec3(0.f); // 월드 접촉점
};

struct PairKey {
	const CCollider* A;
	const CCollider* B;

	bool operator==(const PairKey& other) const noexcept {
		return A == other.A && B == other.B;
	}
};

struct PairKeyHash {
	size_t operator()(const PairKey& k) const noexcept {
		const size_t h1 = std::hash<const void*>{}(k.A);
		const size_t h2 = std::hash<const void*>{}(k.B);
		return h1 ^ (h2 << 1);
	}
};

class PhysicsUtil {
public:
	static constexpr float kEpsilon = 0.000001f;

public:

	static Vec3 GetColliderWorldPosition(const CCollider* collider);
	static Vec3 GetFinalColliderScale(const CCollider* collider);
	static Vec3 GetBoxScaledHalfExtents(const CBoxCollider* box);
	static float GetSphereScaledRadius(const CSphereCollider* sphere);
	static float GetCapsuleScaledRadius(const CCapsuleCollider* capsule);
	static float GetComponent(const Vec3& v, int idx);
	static Vec3 GetAxisByIndex(const OBB& box, int idx);
	static Vec3 GetBodyWorldCenter(const PhysicsBodyEntry& body);
	static float GetBodyInvMass(const PhysicsBodyEntry& body);
	static float GetCapsuleScaledHalfSegment(const CCapsuleCollider* capsule);
	static Segment GetCapsuleSegment(const CCapsuleCollider* capsule);
	static void GetFaceBasis(
		const OBB& box,
		int axisIndex,
		int sign,
		FaceBasis& outFace);
	static void GetFaceVertices(
		const OBB& box,
		int axisIndex,
		int sign,
		std::vector<Vec3>& outVerts);

	static Vec3 NormalizeSafe(const Vec3& v, const Vec3& fallback = Vec3(0.f, 1.f, 0.f));
	static Vec3 AbsVec3(const Vec3& v);
	static float Clamp01(float v);

	static OBB GetOBB(const CBoxCollider* box);
	static Vec3 WorldToOBBLocalPoint(const OBB& box, const Vec3& worldPoint);
	static Vec3 OBBLocalToWorldPoint(const OBB& box, const Vec3& localPoint);
	static Vec3 ClosestPointOnOBB(const OBB& box, const Vec3& worldPoint);

	static bool CheckSphereSphere(
		const CSphereCollider* a,
		const CSphereCollider* b,
		Vec3& outNormal,
		float& outPenetration);
	static bool CheckSphereBox(
		const CSphereCollider* sphere,
		const CBoxCollider* box,
		Vec3& outNormal,
		float& outPenetration);
	static bool CheckBoxBox(
		const CBoxCollider* a,
		const CBoxCollider* b,
		Vec3& outNormal,
		float& outPenetration);

	static void ClosestPtSegmentSegment(
		const Vec3& p1,
		const Vec3& q1,
		const Vec3& p2,
		const Vec3& q2,
		float& s,
		float& t,
		Vec3& c1,
		Vec3& c2);
	static Vec3 ClosestPointOnSegment(const Vec3& p, const Vec3& a, const Vec3& b);

	static Vec3 MulInvInertiaWorld(const CRigidbody* rb, const Vec3& v);

	static float ComputeImpulseDenominator(
		CRigidbody* rbA, CRigidbody* rbB,
		float invMassA, float invMassB,
		const Vec3& ra, const Vec3& rb,
		const Vec3& dir);

	static void ApplyImpulseAtPoint(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Vec3& point,
		const Vec3& impulse);

	static bool CheckCapsuleSphere(
		const CCapsuleCollider* capsule,
		const CSphereCollider* sphere,
		Vec3& outNormal,
		float& outPenetration);
	static bool CheckCapsuleCapsule(
		const CCapsuleCollider* a,
		const CCapsuleCollider* b,
		Vec3& outNormal,
		float& outPenetration);
	static bool CheckCapsuleBox(
		const CCapsuleCollider* capsule,
		const CBoxCollider* box,
		Vec3& outNormal,
		float& outPenetration);

	static void PositionalCorrection(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Vec3& normal,
		float penetration);

	static float ComputeRestitution(CRigidbody* a, CRigidbody* b, float velAlongNormal);
	static float ComputeFriction(CRigidbody* a, CRigidbody* b);

	static void ClipPolygonAgainstPlane(
		std::vector<Vec3>& poly,
		const Vec3& planeN,
		const Vec3& planeOrigin,
		float planeLimit);

	static int FindBestAxis(const OBB& box, const Vec3& dir, float& outAbsDot);
	static int FindIncidentFaceAxis(const OBB& box, const Vec3& referenceNormal, int& outSign);

	static void ReduceContactPoints(std::vector<Contact>& contacts, size_t maxCount);

	static void GenerateBoxBoxManifold(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Vec3& normalAB,
		float pairPenetration,
		std::vector<Contact>& outContacts);

	static void BuildContactBasis(const Vec3& normal, Vec3& outT1, Vec3& outT2);

	static void ApplyFrictionImpulseAlong(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Contact& c,
		const Vec3& dir,
		float normalImpulseMagnitude);
	static void ResolveImpulse(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Contact& c,
		float restitutionScale = 1.f);

	static bool GenerateContact(PhysicsBodyEntry& a, PhysicsBodyEntry& b, Contact& outContact);

	static Vec3 ComputeContactPoint(const PhysicsBodyEntry& a, const PhysicsBodyEntry& b, const Vec3& normal);

	static void UpdateBodyInertiaFromCollider(PhysicsBodyEntry& body);

	static bool IsRollingShape(const PhysicsBodyEntry& body);
	static float GetRollingRadius(const PhysicsBodyEntry& body);

	static void StabilizeRollingAngularVelocity(std::vector<Contact>& contacts);

	static PairKey MakePairKey(const Contact& c);
	static Vec3 GetVelocityAtPoint(const PhysicsBodyEntry& body, const Vec3& point);
	static float ApplyNormalImpulseAtContact(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Contact& c,
		float restitutionScale = 1.f);
	static void ApplyPairFrictionImpulse(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Vec3& point,
		const Vec3& normal,
		float normalImpulseSum);

	static bool IsBoxBoxPair(const PhysicsBodyEntry& a, const PhysicsBodyEntry& b);
	static Vec3 ComputeAveragePoint(const std::vector<Contact*>& manifold);
	static Vec3 ComputeAverageNormal(const std::vector<Contact*>& manifold);
	static void RemoveRelativeTangentialVelocityAtPoint(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		const Vec3& point,
		const Vec3& normal,
		float maxImpulseScale);
	static void StabilizeRestingBoxContacts(std::vector<Contact>& contacts);

	static void StabilizeBoxBoxLanding(std::vector<Contact>& contacts);

	static bool IsMostlyStaticSupport(
		const PhysicsBodyEntry& a,
		const PhysicsBodyEntry& b,
		PhysicsBodyEntry*& dynBody,
		PhysicsBodyEntry*& supportBody);

	static bool IsDynamicBody(const PhysicsBodyEntry& body);
	static bool IsStaticBody(const PhysicsBodyEntry& body);

	static bool PickDynamicVsSupport(
		PhysicsBodyEntry& a,
		PhysicsBodyEntry& b,
		PhysicsBodyEntry*& outDynamic,
		PhysicsBodyEntry*& outSupport,
		Vec3& inOutNormal);

	static Vec3 GetBodyUpAxis(const PhysicsBodyEntry& body);

	static Vec3 GetBestAlignedBodyAxis(const PhysicsBodyEntry& body, const Vec3& targetNormal);
};