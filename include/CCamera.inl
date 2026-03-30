#ifndef __CCAMERA_INL__
#define __CCAMERA_INL__

void CCamera::LayerCheckAll() noexcept { mLayerMask = 0xFFFFFFFF; }

void CCamera::LayerCheckClear() noexcept { mLayerMask = 0; }

void CCamera::LayerCheck(int index) noexcept { mLayerMask ^= (1 << index); }

float CCamera::GetNear() const noexcept { return mNear; }

void CCamera::SetNear(float nearZ) noexcept { mNear = nearZ; }

float CCamera::GetFar() const noexcept { return mFar; }

void CCamera::SetFar(float farZ) noexcept { mFar = farZ; }

float CCamera::GetAspectRatio() const noexcept { return mAspectRatio; }

void CCamera::SetAspectRatio(float aspectRatio) noexcept { mAspectRatio = aspectRatio; }

float CCamera::GetFovY() const noexcept { return mFovY; }

void CCamera::SetFovY(float fovY) noexcept { mFovY = fovY; }

float CCamera::GetWidth() const noexcept { return mWidth; }

void CCamera::SetWidth(float width) noexcept { mWidth = width; }

float CCamera::GetOrthoScale() const noexcept { return mOrthoScale; }

void CCamera::SetOrthoScale(float orthoScale) noexcept { mOrthoScale = orthoScale; }

const Mat4& CCamera::GetViewMatrix() const noexcept { return mViewMatrix; }

const Mat4& CCamera::GetProjMatrix() const noexcept { return mProjMatrix; }

EProjection::Type CCamera::GetProjectionType() const noexcept { return mProjType; }

void CCamera::SetProjectionType(EProjection::Type type) noexcept { mProjType = type; }

UINT CCamera::GetLayerMask() const noexcept { return mLayerMask; }

const std::vector<RenderObject>& CCamera::GetRenderDomainObjects(ERenderDomain::Type domain) const noexcept {
	return mRenderDomains[domain];
}

#endif // __CCAMERA_INL__