#include "pch.h"
#include "CTransform.hpp"

#include "GameObject.hpp"

using namespace DirectX;

CTransform::CTransform() 
	: Component{ EComponent::E_Transform }
	, mPosition{}
	, mRotation{}
	, mScale{ 1.f }
	, mDependency{ ETrasnformDependency::E_All }
	, mbChanged{ true }{
	mDirections[ETransformDirection::E_Right] = Vec3(1.f, 0.f, 0.f);
	mDirections[ETransformDirection::E_Up] = Vec3(0.f, 1.f, 0.f);
	mDirections[ETransformDirection::E_Forward] = Vec3(0.f, 0.f, 1.f);
}

CTransform::~CTransform() {}

bool CTransform::Final() {
	// 크기 -> 회전 -> 이동
	Mat4 transMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
	Mat4 rotMat= XMMatrixRotationX(mRotation.x)
		* XMMatrixRotationY(mRotation.y)
		* XMMatrixRotationZ(mRotation.z);
	Mat4 scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);

	// 방향벡터 계산
	mDirections[ETransformDirection::E_Right] = Vec3(1.f, 0.f, 0.f);
	mDirections[ETransformDirection::E_Up] = Vec3(0.f, 1.f, 0.f);
	mDirections[ETransformDirection::E_Forward] = Vec3(0.f, 0.f, 1.f);

	// 변환행렬을 적용할 Vec3 벡터를 좌표성 데이터로 본다(동차좌표 1로 확장 -> 4행 이동정보 적용)
	// XMVector3TransformCoord(m_Dir[(UINT)DIR::RIGHT], matRot);

	// 변환행렬을 적용할 Vec3 벡터를 방향성 데이터로 본다(동차좌표 0로 확장 -> 4행 이동정보 무시)
	mDirections[ETransformDirection::E_Right] = XMVector3TransformNormal(
		mDirections[ETransformDirection::E_Right], rotMat);
	mDirections[ETransformDirection::E_Up] = XMVector3TransformNormal(
		mDirections[ETransformDirection::E_Up], rotMat);
	mDirections[ETransformDirection::E_Forward] = XMVector3TransformNormal(
		mDirections[ETransformDirection::E_Forward], rotMat);


	// 월드행렬 계산 ( 크기 x 회전 x 이동 )
	mWoldMatrix = scaleMat * rotMat * transMat;

	// 부모 오브젝트가 있었다면
	if (GetOwner()->GetParent() != nullptr) {
		// 부모 오브젝트의 크기에 영향을 받지 않겠다.
		if (!(mDependency & ETrasnformDependency::E_Scale)) {
			Vec3 parentScale = GetOwner()->GetParent()->Transform()->GetWorldScale();
			Mat4 parentScaleMat = XMMatrixScaling(parentScale.x, parentScale.y, parentScale.z);
			Mat4 parentScaleInvMat = XMMatrixInverse(nullptr, parentScaleMat);

			mWoldMatrix = mWoldMatrix * parentScaleInvMat * GetOwner()->GetParent()->Transform()->GetWorldMatrix();
		}
		// 부모 오브젝트의 크기에 영향을 받는다.
		else
			mWoldMatrix *= GetOwner()->GetParent()->Transform()->GetWorldMatrix();
	}

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

	return true;
}

const Vec3& CTransform::GetWorldScale() const {
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