#ifndef __CRENDERCOMPONENT_INL__
#define __CRENDERCOMPONENT_INL__

Ptr<AMesh> CRenderComponent::GetMesh() const noexcept { return mMesh; }

Ptr<AMaterial> CRenderComponent::GetMaterial() const noexcept { return mMaterial; }

#endif // __CRENDERCOMPONENT_INL__