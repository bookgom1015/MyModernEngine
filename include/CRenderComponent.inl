#ifndef __CRENDERCOMPONENT_INL__
#define __CRENDERCOMPONENT_INL__

Ptr<AMesh> CRenderComponent::GetMesh() const noexcept { return mMesh; }

Ptr<AMaterial> CRenderComponent::GetMaterial(size_t index) const noexcept { 
	return mMaterialSlots.empty() ? nullptr : mMaterialSlots[index].Material;
}

#endif // __CRENDERCOMPONENT_INL__