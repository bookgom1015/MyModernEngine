#ifndef __CRENDERCOMPONENT_INL__
#define __CRENDERCOMPONENT_INL__

Ptr<AMesh> CRenderComponent::GetMesh() const noexcept { return mMesh; }

Ptr<AMaterial> CRenderComponent::GetMaterial() const noexcept { return mMaterial; }

Ptr<AMaterial> CRenderComponent::GetSharedMaterial() const noexcept { return mSharedMaterial; }

#endif // __CRENDERCOMPONENT_INL__