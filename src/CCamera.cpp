#include "pch.h"
#include "CCamera.hpp"

#include RENDERER_HEADER
#include "LevelManager.hpp"

#include "CTransform.hpp"
#include "CSkeletalMeshRender.hpp"
#include "CSkySphereRender.hpp"

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

bool CCamera::Begin() { return true; }

bool CCamera::Final() {
	// 뷰 행렬을 계산합니다.
	// 카메라의 위치를 가져옵니다.
	Vec3 vPos = Transform()->GetRelativePosition();

	// 카메라의 위치를 원점으로 옮기는 이동 행렬을 구성합니다.
	Mat4 transMat = Identity4x4;
	transMat._41 = -vPos.x;
	transMat._42 = -vPos.y;
	transMat._43 = -vPos.z;

	// 카메라의 Right, Up, Forward 방향을 각각 뷰 공간의 X, Y, Z축에 맞춥니다.
	// 세 방향 벡터가 정규직교 기저를 이루므로, 기저 행렬의 전치가 역행렬이 됩니다.
	// 같은 방향 벡터끼리의 내적은 1이고, 서로 다른 방향 벡터끼리의 내적은 0입니다.
	// 따라서 각 방향 벡터를 열에 배치하여 뷰 회전 행렬을 구성합니다.
	Vec3 vR = Transform()->GetDirection(ETransformDirection::E_Right);
	Vec3 vU = Transform()->GetDirection(ETransformDirection::E_Up);
	Vec3 vF = Transform()->GetDirection(ETransformDirection::E_Forward);

	Mat4 rotMat = Identity4x4;
	rotMat._11 = vR.x;  rotMat._12 = vU.x;  rotMat._13 = vF.x;
	rotMat._21 = vR.y;	rotMat._22 = vU.y;	rotMat._23 = vF.y;
	rotMat._31 = vR.z;	rotMat._32 = vU.z;	rotMat._33 = vF.z;

	// 이동과 회전을 순서대로 적용하여 월드 공간을 뷰 공간으로 변환합니다.
	mViewMatrix = transMat * rotMat;

	// 투영 방식에 따라 투영 행렬을 계산합니다.
	if (mProjType == EProjection::E_Orthographic) {
		// 직교 투영(Orthographic) 행렬을 계산합니다.	
		mProjMatrix = XMMatrixOrthographicLH(
			mWidth * mOrthoScale, (mWidth / mAspectRatio) * mOrthoScale, mNear, mFar);
	}
	else {
		// 원근 투영(Perspective) 행렬을 계산합니다.
		mProjMatrix = XMMatrixPerspectiveFovLH(
			mFovY, mAspectRatio, mNear, mFar);
	}

    return true;
}

void CCamera::SortRenderObjects() {
	for (size_t i = 0; i < ERenderDomain::Count; ++i)
		mRenderDomains[i].clear();

	Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
	if (currLevel == nullptr) return;

	for (UINT i = 0; i < MAX_LAYER; ++i) {
		if (!(mLayerMask & (1 << i))) continue;

		Layer* layer = currLevel->GetLayer(i);
		const auto& objects = layer->GetAllObjects();

		for (size_t j = 0, end = objects.size(); j < end; ++j) {
			auto renderComp = objects[j]->GetRenderComponent();
			if (renderComp == nullptr || renderComp->GetMesh() == nullptr)
				continue;

			auto mesh = renderComp->GetMesh();
			const auto& sourcePrimitives = mesh->GetMeshPrimitives();

			UINT staticLocalIndex = 0;
			UINT skinnedLocalIndex = 0;

			for (UINT sourceIndex = 0; sourceIndex < sourcePrimitives.size(); ++sourceIndex) {
				const auto& prim = sourcePrimitives[sourceIndex];

				auto mat = renderComp->GetMaterial(sourceIndex);

				ERenderDomain::Type domain = ERenderDomain::E_Opaque;
				if (mat != nullptr)
					domain = mat->GetRenderDomain();

				auto transform = objects[j]->Transform();

				RenderObjectDesc ro{};
				ro.World = transform->GetWorldMatrix();
				ro.PrevWorld = transform->GetPrevWorldMatrix();
				ro.StaticMeshKey = mesh->GetKey();
				ro.SkinnedMeshKey = mesh->GetKey();
				ro.Material.Albedo = renderComp->GetAlbedo(sourceIndex);
				ro.Material.Roughness = renderComp->GetRoughness(sourceIndex);
				ro.Material.Metalness = renderComp->GetMetalic(sourceIndex);
				ro.Material.Specular = renderComp->GetSpecular(sourceIndex);
				if (mat != nullptr) {
					if (auto map = mat->GetAlbedoMap(); map != nullptr) ro.Material.AlbedoMapKey = map->GetKey();
					if (auto map = mat->GetNormalMap(); map != nullptr) ro.Material.NormalMapKey = map->GetKey();
				}
				if (auto sky = objects[j]->SkySphereRender(); sky != nullptr) {
					if (auto map = sky->GetEnvironmentCubeMap(); map != nullptr) ro.EnvironmentMapKey = map->GetKey();
				}
				auto skeletal = objects[j]->SkeletalMeshRender();
				if (prim.VertexType == EVertex::E_Static && skeletal != nullptr && prim.NodeIndex >= 0) {
					ro.World = skeletal->GetNodeGlobalPose(prim.NodeIndex) * ro.World;
					ro.PrevWorld = skeletal->GetPrevNodeGlobalPose(prim.NodeIndex) * ro.PrevWorld;
				}
				if (prim.VertexType == EVertex::E_Skinned && skeletal != nullptr) {
					ro.SkinIndex = prim.SkinIndex;
					ro.BonePalette = skeletal->GetPalette(prim.SkinIndex);
					ro.PrevBonePalette = skeletal->GetPrevPalette(prim.SkinIndex);
				}
				ro.SourcePrimitiveIndex = static_cast<int>(sourceIndex);
				if (prim.VertexType == EVertex::E_Static) {
					ro.StaticPrimitiveIndex = static_cast<int>(staticLocalIndex);
					ro.SkinnedPrimitiveIndex = -1;
					++staticLocalIndex;
				}
				else if (prim.VertexType == EVertex::E_Skinned) {
					ro.StaticPrimitiveIndex = -1;
					ro.SkinnedPrimitiveIndex = static_cast<int>(skinnedLocalIndex);
					++skinnedLocalIndex;
				}
				else {
					continue;
				}

				mRenderDomains[domain].push_back(ro);
			}
		}
	}
}

void CCamera::SubmitRenderData() {
	RenderCameraDesc camera{};
	camera.FovY = mFovY;
	camera.NearZ = mNear;
	camera.FarZ = mFar;
	camera.Position = GetCameraPosition();
	camera.Direction = Transform()->GetDirection(ETransformDirection::E_Forward);
	camera.Up = Transform()->GetDirection(ETransformDirection::E_Up);
	camera.ViewMatrix = mViewMatrix;
	camera.ProjMatrix = mProjMatrix;
	camera.UnitViewMatrix = GetUnitViewMatrix();
	camera.OrthoProjMatrix = GetOrthoProjMatrix();
	camera.ProjType = mProjType;

	RENDERER->UpdateCamera(camera);

	SortRenderObjects();

	for (int domain = 0; domain < ERenderObjectDomain::Count; ++domain)
		RENDERER->UpdateRenderObjects(static_cast<ERenderObjectDomain::Type>(domain), mRenderDomains[domain]);
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

	// 월드 → 뷰
	Vec4 vpos = XMVector3TransformCoord(pos, mViewMatrix);
	// 뷰 → 클립
	Vec4 cpos = XMVector3TransformCoord(vpos, mProjMatrix);
	
	// 동차 나누기 → NDC
	cpos /= cpos.w;

	// NDC → 화면
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