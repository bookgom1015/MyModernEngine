#ifndef __CRENDERCOMPONENT_INL__
#define __CRENDERCOMPONENT_INL__

Ptr<AMesh> CRenderComponent::GetMesh() const noexcept { return mMesh; }

void CRenderComponent::SetMesh(Ptr<AMesh> mesh) noexcept { mMesh = mesh; }

Ptr<AMaterial> CRenderComponent::GetMaterial() const noexcept { return mMaterial; }

void CRenderComponent::SetMaterial(Ptr<AMaterial> material) noexcept { mMaterial = material; }

Ptr<AMaterial> CRenderComponent::GetSharedMaterial() const noexcept { return mSharedMaterial; }

#endif // __CRENDERCOMPONENT_INL__