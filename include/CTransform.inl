#ifndef __CTRANSFORM_INL__
#define __CTRANSFORM_INL__

const Vec3& CTransform::GetRelativePosition() const noexcept { return mPosition; }

const Vec3& CTransform::GetRelativeRotation() const noexcept { return mRotation; }

const Vec3& CTransform::GetRelativeScale() const noexcept { return mScale; }

void CTransform::SetRelativePosition(const Vec3& position) { mPosition = position; }

void CTransform::SetRelativeRotation(const Vec3& rotation) {
	mRotation = rotation;
	SyncQuaternionFromEuler();
}

void CTransform::AddRelativePosition(const Vec3& delta) { mPosition += delta; }

void CTransform::AddRelativeRotation(const Vec3& delta) {
	mRotation += delta;
	SyncQuaternionFromEuler();
}

const Mat4& CTransform::GetWorldMatrix() const noexcept { return mWorldMatrix; }

const Mat4& CTransform::GetPrevWorldMatrix() const noexcept { return mPrevWorldMatrix; }

ETrasnformDependency::Type CTransform::GetDependency() const noexcept { return mDependency; }

void CTransform::SetDependency(ETrasnformDependency::Type dependency) { mDependency = dependency; }

const Vec3& CTransform::GetDirection(ETransformDirection::Type dir) const {
	return mDirections[dir];
}

#endif // __CTRANSFORM_INL__
