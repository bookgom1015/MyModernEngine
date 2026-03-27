#include "pch.h"
#include "CCamera.hpp"

#include RENDERER_HEADER
#include "LevelManager.hpp"

#include "CTransform.hpp"

using namespace DirectX;

CCamera::CCamera() : 
	Component{ EComponent::E_Camera }
	, mNear{ 0.1f }
	, mFar{ 100.f }
	, mAspectRatio{ 1.f }
	, mFovY { PITwo }
	, mWidth{ 1.f }
	, mOrthoScale{ 1.f }
	, mViewMatrix{ Identity4x4 }
	, mProjMatrix{ Identity4x4 }
	, mProjType{ EProjection::E_Perspective }
	, mLayerMask{ 0xFFFFFFFF } {}

CCamera::~CCamera() {}

bool CCamera::Begin() {
	RENDERER->SetCamera(this);

    return true;
}

bool CCamera::Final() {
	// 뷰 행렬 계산
	// 카메라의 위치
	Vec3 vPos = Transform()->GetRelativePosition();

	// 이동 ( 카메라 위치를 원점으로 되돌리는 만큼의 이동행렬)
	Mat4 transMat = Identity4x4;
	transMat._41 = -vPos.x;
	transMat._42 = -vPos.y;
	transMat._43 = -vPos.z;

	// View 행렬 회전
	//  - 카메라가 바라보는 방향을 z 축이 되도록 회전하는 부분이 추가
	// 카메라의 Right, Up, Front 방향 벡터에 회전행렬을 곱하면 이게 다시 x축, y축, z 축이 되는 회전행을 구해야 함

	// vR				  ( 1 0 0)
	// vU    x    R   =   ( 0 1 0)
	// vZ				  ( 0 0 1)

	//			vR
	// R 행렬은  vU     의 역핼렬
	//			vZ	

	//	vR
	//  vU   행렬은 행 끼리의 관계가 직교상태이기 때문에, 전치(Transpose) 를 통해서 역행렬을 쉽게 구할 수 있음
	//	vZ

	// 전치한 행렬과 곱해보면, 자기자신과 내적을 한 경우 결과가 1, 다른 직교벡터랑 내적을 한 경우 0 이 나오기 때문
	// 내적은 각 성분끼리의 곱을 합친 결과, 
	// 내적 결과값의 의미는 (벡터의 길이) x (길이) x (두 벡터가 이루는 각도의 cos 값) 

	//		 ( vR.x   vU.x   vF.x    0 ) 
	// R ==  ( vR.y   vU.y   vF.y    0 )
	//		 ( vR.z   vU.z   vF.z    0 )
	//       (   0     0       0     1 )

	Vec3 vR = Transform()->GetDirection(ETransformDirection::E_Right);
	Vec3 vU = Transform()->GetDirection(ETransformDirection::E_Up);
	Vec3 vF = Transform()->GetDirection(ETransformDirection::E_Forward);

	Mat4 rotMat = Identity4x4;
	rotMat._11 = vR.x;  rotMat._12 = vU.x;  rotMat._13 = vF.x;
	rotMat._21 = vR.y;	rotMat._22 = vU.y;	rotMat._23 = vF.y;
	rotMat._31 = vR.z;	rotMat._32 = vU.z;	rotMat._33 = vF.z;

	// 카메라가 원점인 공간으로 이동, 카메라가 바라보는 방향을 z 축으로 회전하는 회전을 적용
	mViewMatrix = transMat * rotMat;

	// 투영행렬
	if (mProjType == EProjection::E_Orthographic) {
		// 직교투영(Orthographic) 행렬 계산	
		mProjMatrix = XMMatrixOrthographicLH(
			mWidth * mOrthoScale, (mWidth / mAspectRatio) * mOrthoScale, mNear, mFar);
	}
	else {
		// 원근투영(Perspective)
		mProjMatrix = XMMatrixPerspectiveFovLH(
			mFovY, mAspectRatio, mNear, mFar);
	}

    return true;
}

void CCamera::SortObjects() {
	// 렌더링 할 물체들을 정렬한다.
	for (size_t i = 0; i < ERenderDomain::Count; ++i) 
		mRenderDomains[i].clear();

	Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
	if (currLevel == nullptr) return;

	for (UINT i = 0; i < MAX_LAYER; ++i) {
		// 카메라가 레이어를 볼 수 있어야 함
		if (!(mLayerMask & (1 << i))) continue;

		// 레이어에 소속된 모든 오브젝트를 가져온다
		Layer* layer = currLevel->GetLayer(i);
		const auto& objects = layer->GetAllObjects();

		for (size_t j = 0, end = objects.size(); j < end; ++j) {
			// 오브젝트가 렌더링을 할 수 있는 상태인지 확인
			if (objects[j]->GetRenderComponent() == nullptr
				|| objects[j]->GetRenderComponent()->GetMesh() == nullptr)
				continue;

			auto mat = objects[j]->GetRenderComponent()->GetMaterial();
			ERenderDomain::Type domain = ERenderDomain::E_Opaque;
			if (mat != nullptr) domain = mat->GetRenderDomain();

			mRenderDomains[domain].push_back(objects[j].Get());
		}
	}
}

Vec3 CCamera::GetScreenToWorld(const Vec2& screenPos, Vec2 screenSize) const {
	auto temp = screenPos / screenSize;
	temp = temp * 2.f - Vec2(1.f, 1.f);

	Vec4 ndc(temp.x, temp.y, 0.f, 1.f);
	ndc.y *= -1.f;

	auto detView = XMMatrixDeterminant(mViewMatrix);
	auto detProj = XMMatrixDeterminant(mProjMatrix);
	auto invView = XMMatrixInverse(&detView, mViewMatrix);
	auto invProj = XMMatrixInverse(&detProj, mProjMatrix);

	Vec4 vpos = XMVector3TransformCoord(ndc, invProj);
	vpos /= vpos.w;

	Vec4 wpos = XMVector3TransformCoord(vpos, invView);

	return Vec3(wpos.x, wpos.y, wpos.z);
}

Vec2 CCamera::GetWorldToScreen(const Vec3& worldPos, Vec2 screenSize) const {
	Vec4 pos(worldPos.x, worldPos.y, worldPos.z, 1.f);

	// world → view
	Vec4 vpos = XMVector3TransformCoord(pos, mViewMatrix);
	// view → clip
	Vec4 cpos = XMVector3TransformCoord(vpos, mProjMatrix);

	// perspective divide → NDC
	cpos /= cpos.w;

	// NDC → screen
	Vec2 screen;
	screen.x = (cpos.x + 1.f) * 0.5f * screenSize.x;
	screen.y = (1.f - cpos.y) * 0.5f * screenSize.y;

	return screen;
}

Vec3 CCamera::GetCameraPosition() {
	return Transform()->GetRelativePosition();
}

Mat4 CCamera::GetUnitViewMatrix() {
	auto pos = Transform()->GetDirection(ETransformDirection::E_Forward);
	pos *= -4.f;

	auto up = Transform()->GetDirection(ETransformDirection::E_Up);

	return XMMatrixLookAtLH(pos, Vec4(0.f, 0.f, 0.f, 1.f), up);
}

Mat4 CCamera::GetOrthoProjMatrix() {
	return XMMatrixOrthographicLH(4.f, (4.f / mAspectRatio), 0.01f, 8.f);
}	

bool CCamera::SaveToLevelFile(FILE* const pFile) { return true; }

bool CCamera::LoadFromLevelFile(FILE* const pFile) { return true; }