#include "pch.h"
#include "CTransform.hpp"

#include "GameObject.hpp"

using namespace DirectX;

namespace {
	static Vec3 NormalizeEulerDegrees(const Vec3& euler) {
		Vec3 out = euler;
		auto NormalizeAngle = [](float a) {
			while (a > 180.f) a -= 360.f;
			while (a < -180.f) a += 360.f;
			return a;
			};
		out.x = NormalizeAngle(out.x);
		out.y = NormalizeAngle(out.y);
		out.z = NormalizeAngle(out.z);
		return out;
	}
}

CTransform::CTransform()
	: Component{ EComponent::E_Transform }
	, mPosition{}
	, mRotation{}
	, mScale{ 1.f }
	, mWorldMatrix{ XMMatrixIdentity() }
	, mPrevWorldMatrix{ XMMatrixIdentity() }
	, mRotationQ{ 0.f, 0.f, 0.f, 1.f }
	, mDependency{ ETrasnformDependency::E_All } {
	mDirections[ETransformDirection::E_Right] = Vec3(1.f, 0.f, 0.f);
	mDirections[ETransformDirection::E_Up] = Vec3(0.f, 1.f, 0.f);
	mDirections[ETransformDirection::E_Forward] = Vec3(0.f, 0.f, 1.f);

	SyncQuaternionFromEuler();
}

CTransform::~CTransform() {}

bool CTransform::Final() {
	mPrevWorldMatrix = mWorldMatrix;
	UpdateWorldMatrixImmediate();

	return true;
}

bool CTransform::SaveToLevelFile(FILE* const pFile) {
	fwrite(&mPosition, sizeof(Vec3), 1, pFile);
	fwrite(&mRotation, sizeof(Vec3), 1, pFile);
	fwrite(&mScale, sizeof(Vec3), 1, pFile);
	fwrite(&mDependency, sizeof(ETrasnformDependency::Type), 1, pFile);

	return true;
}

bool CTransform::LoadFromLevelFile(FILE* const pFile) {
	fread(&mPosition, sizeof(Vec3), 1, pFile);
	fread(&mRotation, sizeof(Vec3), 1, pFile);
	fread(&mScale, sizeof(Vec3), 1, pFile);
	fread(&mDependency, sizeof(ETrasnformDependency::Type), 1, pFile);

	SyncQuaternionFromEuler();
	return true;
}

Vec3 CTransform::GetWorldScale() const {
	Vec3 scale = mScale;
	if (!(mDependency & ETrasnformDependency::E_Scale)) return scale;

	Ptr<GameObject> parent = GetOwner()->GetParent();
	while (parent != nullptr) {
		scale *= parent->Transform()->GetRelativeScale();
		if (!(parent->Transform()->GetDependency() & ETrasnformDependency::E_Scale))
			break;

		parent = parent->GetParent();
	}

	return scale;
}

void CTransform::SyncQuaternionFromEuler() {
	const float pitch = XMConvertToRadians(mRotation.x);
	const float yaw = XMConvertToRadians(mRotation.y);
	const float roll = XMConvertToRadians(mRotation.z);

	const XMMATRIX rotMat =
		XMMatrixRotationX(pitch) *
		XMMatrixRotationY(yaw) *
		XMMatrixRotationZ(roll);

	XMVECTOR q = XMQuaternionRotationMatrix(rotMat);
	q = XMQuaternionNormalize(q);
	XMStoreFloat4(&mRotationQ, q);
}

void CTransform::UpdateWorldMatrixImmediate() {
	const XMVECTOR rotQ = XMLoadFloat4(&mRotationQ);

	Mat4 transMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
	Mat4 rotMat = XMMatrixRotationQuaternion(rotQ);
	Mat4 scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);

	mDirections[ETransformDirection::E_Right] = XMVector3TransformNormal(Vec3(1.f, 0.f, 0.f), rotMat);
	mDirections[ETransformDirection::E_Up] = XMVector3TransformNormal(Vec3(0.f, 1.f, 0.f), rotMat);
	mDirections[ETransformDirection::E_Forward] = XMVector3TransformNormal(Vec3(0.f, 0.f, 1.f), rotMat);

	mWorldMatrix = scaleMat * rotMat * transMat;

	if (GetOwner()->GetParent() != nullptr) {
		if (!(mDependency & ETrasnformDependency::E_Scale)) {
			Vec3 parentScale = GetOwner()->GetParent()->Transform()->GetWorldScale();
			Mat4 parentScaleMat = XMMatrixScaling(parentScale.x, parentScale.y, parentScale.z);
			Mat4 parentScaleInvMat = XMMatrixInverse(nullptr, parentScaleMat);

			mWorldMatrix = mWorldMatrix * parentScaleInvMat * GetOwner()->GetParent()->Transform()->GetWorldMatrix();
		}
		else {
			mWorldMatrix *= GetOwner()->GetParent()->Transform()->GetWorldMatrix();
		}
	}
}

void CTransform::IntegrateAngularVelocityWorld(const Vec3& angularVelocityWorld, float dt) {
	if (dt <= 0.f)
		return;

	XMVECTOR q = XMLoadFloat4(&mRotationQ);
	XMVECTOR omega = XMVectorSet(
		angularVelocityWorld.x,
		angularVelocityWorld.y,
		angularVelocityWorld.z,
		0.f);

	XMVECTOR dq = XMQuaternionMultiply(omega, q) * (0.5f * dt);
	q = XMVectorAdd(q, dq);
	q = XMQuaternionNormalize(q);
	XMStoreFloat4(&mRotationQ, q);

	mRotation = NormalizeEulerDegrees(mRotation);
}

Mat4 CTransform::GetTRMatrix() const {
	const XMVECTOR rotQ = XMLoadFloat4(&mRotationQ);

	Mat4 transMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
	Mat4 rotMat = XMMatrixRotationQuaternion(rotQ);

	return rotMat * transMat;
}

void CTransform::SetRelativeScale(const Vec3& scale) {
	mScale = scale;

	if (auto* owner = GetOwner()) {
		if (auto* rb = owner->Rigidbody().Get())
			rb->MarkInertiaDirty();
	}
}
