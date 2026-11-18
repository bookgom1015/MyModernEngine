#ifndef __LAYER_INL__
#define __LAYER_INL__

const std::vector<Ptr<GameObject>>& Layer::GetParents() const noexcept {
	return mParents;
}

const std::vector<Ptr<GameObject>>& Layer::GetAllObjects() const noexcept {
	return mAllObjects;
}

#endif // __LAYER_INL__