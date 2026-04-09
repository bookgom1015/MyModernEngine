#pragma once

#include "Component.hpp"

class GameObject;

class CCamera : public Component {
public:
	CCamera();
	virtual ~CCamera();

public:
	virtual bool Begin() override;
	virtual bool Final() override;

public:
	__forceinline void LayerCheckAll() noexcept;
	__forceinline void LayerCheckClear() noexcept;
	__forceinline void LayerCheck(int index) noexcept;

	void SortRenderObjects();

	Vec3 GetScreenToWorld(const Vec2& screenPos, Vec2 screenSize) const;
	Vec2 GetWorldToScreen(const Vec3& worldPos, Vec2 screenSize) const;

	Vec3 GetCameraPosition();

	Mat4 GetUnitViewMatrix();
	Mat4 GetOrthoProjMatrix();

public:
	CLONE(CCamera);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	__forceinline float GetNear() const noexcept;
	__forceinline void SetNear(float near) noexcept;

	__forceinline float GetFar() const noexcept;
	__forceinline void SetFar(float far) noexcept;

	__forceinline float GetAspectRatio() const noexcept;
	__forceinline void SetAspectRatio(float aspectRatio) noexcept;

	__forceinline float GetFovY() const noexcept;
	__forceinline void SetFovY(float fovY) noexcept;

	__forceinline float GetWidth() const noexcept;
	__forceinline void SetWidth(float width) noexcept;

	__forceinline float GetOrthoScale() const noexcept;
	__forceinline void SetOrthoScale(float orthoScale) noexcept;

	__forceinline const Mat4& GetViewMatrix() const noexcept;
	__forceinline const Mat4& GetProjMatrix() const noexcept;

	__forceinline EProjection::Type GetProjectionType() const noexcept;
	__forceinline void SetProjectionType(EProjection::Type type) noexcept;

	__forceinline UINT GetLayerMask() const noexcept;

	__forceinline const std::vector<RenderObject>& GetRenderDomainObjects(
		ERenderDomain::Type domain) const noexcept;

private:
	float mNear;
	float mFar;
	float mAspectRatio;
	float mFovY;

	float mWidth;
	float mOrthoScale;

	Mat4 mViewMatrix;
	Mat4 mProjMatrix;

	std::vector<RenderObject> mRenderDomains[ERenderDomain::Count];
	std::vector<DebugColliderShape> mDebugColliderShapes;

	EProjection::Type mProjType;
	UINT mLayerMask;
};

#include "CCamera.inl"