#ifndef __CTRANSFORM_INL__
#define __CTRANSFORM_INL__

const Vec3& CTransform::GetRelativePosition() const noexcept { return mPosition; }

const Vec3& CTransform::GetRelativeRotation() const noexcept { return mRotation; }

const Vec3& CTransform::GetRelativeScale() const noexcept { return mScale; }

void CTransform::SetRelativePosition(const Vec3& position) { 
	mPosition = position; 
	mbChanged = true;
}

void CTransform::SetRelativeRotation(const Vec3& rotation) { 
	mRotation = rotation; 
	mbChanged = true;
}

void CTransform::SetRelativeScale(const Vec3& scale) { 
	mScale = scale; 
	mbChanged = true;
}

const Mat4& CTransform::GetWorldMatrix() const noexcept { return mWoldMatrix; }

ETrasnformDependency::Type CTransform::GetDependency() const noexcept { return mDependency; }

void CTransform::SetDependency(ETrasnformDependency::Type dependency) { mDependency = dependency; }

const Vec3& CTransform::GetDirection(ETransformDirection::Type dir) const { 
	return mDirections[dir]; 
}

bool CTransform::IsChanged() const noexcept { return mbChanged; }

void CTransform::ReflectedChanges() noexcept { mbChanged = false; }

#endif // __CTRANSFORM_INL__